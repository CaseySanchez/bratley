#pragma once

#include <cstdint>
#include <type_traits>
#include <tuple>
#include <thread>
#include <future>
#include <chrono>

namespace bratley
{
    enum class execute_status_t : uint8_t
    {
        SUCCESS = 0,
        FAILURE = 1
    };

    // `task_t` stores the `Arrival`, `Cost`, and `Deadline` as compile-time constants to generate schedules
    // Further, `task_t` implements a pure virtual method that is called by `execute_t::execute`

    template <typename Context, uint32_t Arrival, uint32_t Cost, uint32_t Deadline, typename Duration = std::chrono::seconds>
    class task_t
    {
    public:
        using duration_t = Duration;

        static constexpr uint32_t s_arrival { Arrival };
        static constexpr uint32_t s_cost { Cost };
        static constexpr uint32_t s_deadline { Deadline };

        static constexpr duration_t s_arrival_duration { Arrival };
        static constexpr duration_t s_cost_duration { Cost };
        static constexpr duration_t s_deadline_duration { Deadline };

        virtual void task(Context &) = 0;
    };

    // `task_schedule_t` inherits `task_t` and stores the actual `Start` and `Finish` times of the task within the schedule

    template <uint32_t Start, uint32_t Finish, class Task>
    class task_schedule_t : public Task
    {
    public:
        using duration_t = typename Task::duration_t;

        static constexpr uint32_t s_start { Start };
        static constexpr uint32_t s_finish { Finish };

        static constexpr duration_t s_start_duration { Start };
        static constexpr duration_t s_finish_duration { Finish };
    };

    namespace detail
    {
        // `execute_t` iterates through all indices of the tuple, executing each task
        // Each task is executed within a separate thread that waits until the start time
        // The task is monitored from the main thread with a `future` to ensure that it does not exceed its deadline
        // If any deadline is exceeded `execute` returns false and ceases operation

        template <size_t Index>
        struct execute_t
        {
            template <typename Context, typename... Tasks>
            static constexpr execute_status_t execute(Context &context, std::tuple<Tasks...> &tasks, std::chrono::high_resolution_clock::time_point const &start)
            {
                if (execute_t<Index - 1>::execute(context, tasks, start) == execute_status_t::FAILURE) {
                    return execute_status_t::FAILURE;
                }                

                auto const &start_duration = std::remove_reference_t<decltype(std::get<Index>(tasks))>::s_start_duration;
                auto const &finish_duration = std::remove_reference_t<decltype(std::get<Index>(tasks))>::s_finish_duration;

                std::promise<void> promise;
                std::future<void> future = promise.get_future();

                std::thread thread([&]() -> void {
                    std::this_thread::sleep_until(start + start_duration);

                    std::get<Index>(tasks).task(context);

                    promise.set_value();
                });

                std::future_status const future_status = future.wait_until(start + finish_duration);

                thread.join();

                if (future_status == std::future_status::ready) {
                    return execute_status_t::SUCCESS;
                }
                else {
                    return execute_status_t::FAILURE;
                }
            }
        };

        template <>
        struct execute_t<0>
        {
            template <typename Context, typename... Tasks>
            static constexpr execute_status_t execute(Context &context, std::tuple<Tasks...> &tasks, std::chrono::high_resolution_clock::time_point const &start)
            {
                auto const &start_duration = std::remove_reference_t<decltype(std::get<0>(tasks))>::s_start_duration;
                auto const &finish_duration = std::remove_reference_t<decltype(std::get<0>(tasks))>::s_finish_duration;
             
                std::promise<void> promise;
                std::future<void> future = promise.get_future();

                std::thread thread([&]() -> void {
                    std::this_thread::sleep_until(start + start_duration);

                    std::get<0>(tasks).task(context);

                    promise.set_value();
                });

                std::future_status const future_status = future.wait_until(start + finish_duration);

                thread.join();

                if (future_status == std::future_status::ready) {
                    return execute_status_t::SUCCESS;
                }
                else {
                    return execute_status_t::FAILURE;
                }
            }
        };

        // `validate_t` determines whether or not a schedule is valid
        // It does this by first checking if the task has arrived
        // If it has arrived, it checks if the current time plus the cost is less-than-or-equal-to the deadline
        // If it has not arrived, it checks if the arrival time plus the cost is less-than-or-equal-to the deadline
        // It performs a logical AND while unpacking the tasks in the tuple
        // If any one task evaluates to false then the entire schedule is invalid
       
        template <uint32_t Time, typename Task>
        struct validate_t
        {
            static constexpr uint32_t start()
            {
                if constexpr (Task::s_arrival > Time) {
                    return Task::s_arrival;
                }
                else {
                    return Time;
                }
            }

            static constexpr uint32_t finish()
            {
                return start() + Task::s_cost;
            }

            static constexpr bool validate()
            {
                return finish() <= Task::s_deadline;
            }
        };

        // `join_t` joins two tuples into one tuple 
        // e.g.
        // `join_t<std::tuple<int, float>, std::tuple<long, double>>::type`
        // is of type
        // `std::tuple<int, float, long, double>`

        template <typename TupleLhs, typename TupleRhs>
        struct join_t;

        template <typename... TupleLhs, typename... TupleRhs>
        struct join_t<std::tuple<TupleLhs...>, std::tuple<TupleRhs...>>
        {
            using type = std::tuple<TupleLhs..., TupleRhs...>;
        };

        // `concat_t` concatenates multiple tuples with the help of `join_t`
        // e.g.
        // `concat_t<std::tuple<int>, std::tuple<float>, std::tuple<long>, std::tuple<double>>::type`
        // is of type
        // `std::tuple<int, float, long, double>`

        template <typename... Tuples>
        struct concat_t;

        template <typename TupleHead, typename... TupleTail>
        struct concat_t<TupleHead, TupleTail...>
        {
            using concatenated = typename concat_t<TupleTail...>::type;

            using type = typename join_t<TupleHead, concatenated>::type;
        };

        template <typename TupleLhs, typename TupleRhs>
        struct concat_t<TupleLhs, TupleRhs>
        {
            using type = typename join_t<TupleLhs, TupleRhs>::type;
        };

        // `rotate_t` rotates the types within a tuple such that the first element becomes the last element
        // and subsequent elements are shifted to the left
        // e.g.
        // `rotate_t<std::tuple<int, float, long, double>>::type`
        // is of type
        // `std::tuple<float, long, double, int>`

        template <typename Tuple = void>
        struct rotate_t
        {
            using type = std::tuple<>;
        };

        // Specialization for the n-tuple case
        template <typename TaskHead, typename... TaskTail>
        struct rotate_t<std::tuple<TaskHead, TaskTail...>>
        {
            using type = std::tuple<TaskTail..., TaskHead>;
        };

        // Specialization for the 1-tuple case
        template <typename TaskHead>
        struct rotate_t<std::tuple<TaskHead>>
        {
            using type = std::tuple<TaskHead>;
        };

        // `prepend_t` prepends a 1-tuple to the n-tuple contents of an n-tuple
        // e.g.
        // `prepend_t<std::tuple<int>, std::tuple<std::tuple<float>, std::tuple<long>, std::tuple<double>>>::type`
        // is of type
        // `std::tuple<std::tuple<int, float>, std::tuple<int, long>, std::tuple<int, double>>`

        template <typename TaskHead, typename TaskTail>
        struct prepend_t;

        template <typename TaskHead, typename... TaskTail>
        struct prepend_t<std::tuple<TaskHead>, std::tuple<TaskTail...>>
        {
            using type = std::tuple<typename join_t<std::tuple<TaskHead>, TaskTail>::type...>;
        };

        // `schedule_t` generates the branches of the schedule tree as a tuple of tuples
        // If a branch is invalid the tuple will stop growing where the branch becomes invalid
        // The only valid branches in the tuple will be of the same size as the number of tasks to be scheduled

        template <uint32_t Time, uint32_t Index, typename Tuple = void>
        struct schedule_t
        {
            using type = std::tuple<>;
        };

        template <uint32_t Time, uint32_t Index, typename TaskHead, typename... TaskTail>
        struct schedule_t<Time, Index, std::tuple<TaskHead, TaskTail...>>
        {
            using validator = validate_t<Time, TaskHead>;
            using task_validated = task_schedule_t<validator::start(), validator::finish(), TaskHead>;

            using tasks = std::tuple<TaskHead, TaskTail...>;
            using rotated = typename rotate_t<tasks>::type;

            using future_branches = typename schedule_t<validator::finish(), sizeof...(TaskTail), std::tuple<TaskTail...>>::type;
            using present_branches = typename schedule_t<Time, Index - 1, rotated>::type;

            using type = std::conditional_t<
                validator::validate(),
                typename join_t<
                    typename prepend_t<
                        std::tuple<task_validated>, 
                        future_branches
                    >::type,
                    present_branches
                >::type,
                std::tuple<std::tuple<>>
            >;
        };

        template <uint32_t Time, typename TaskHead, typename... TaskTail>
        struct schedule_t<Time, 0, std::tuple<TaskHead, TaskTail...>>
        {
            using type = std::tuple<>;
        };

        template <uint32_t Time, typename TaskHead>
        struct schedule_t<Time, 0, std::tuple<TaskHead>>
        {
            using validator = validate_t<Time, TaskHead>;
            using task_validated = task_schedule_t<validator::start(), validator::finish(), TaskHead>;

            using type = std::tuple<std::tuple<task_validated>>;
        };

        // `prune_t` prunes branches that do not contain the total number of tasks

        template <uint32_t Size, typename Schedules>
        struct prune_t;

        template <uint32_t Size, typename... Schedules>
        struct prune_t<Size, std::tuple<Schedules...>>
        {
            template <typename Schedule>
            using prune = std::conditional_t<
                Size == std::tuple_size_v<Schedule>, 
                std::tuple<Schedule>, 
                std::tuple<>
            >;

            using type = typename concat_t<prune<Schedules>...>::type;
        };
    }

    template <typename... Tasks>
    static constexpr auto schedule()
    {
        using schedule = typename detail::schedule_t<0, sizeof...(Tasks), std::tuple<Tasks...>>::type;
        using prune = typename detail::prune_t<sizeof...(Tasks), schedule>::type;

        return prune { };
    }

    template <typename Context, typename... Tasks>
    static constexpr execute_status_t execute(Context &context, std::tuple<Tasks...> tasks)
    {
        std::chrono::high_resolution_clock::time_point const start = std::chrono::high_resolution_clock::now();

        return detail::execute_t<sizeof...(Tasks) - 1>::execute(context, tasks, start);
    }
}