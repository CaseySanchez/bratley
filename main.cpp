#include <iostream>

#include "bratley.hpp"

struct context_t
{
    int m_data;
};

class task_1_t : public bratley::task_t<context_t, 4, 2, 7>
{
public:
    virtual void task(context_t &context) override
    {
        std::cout << "Executing task 1" << std::endl;

        // Modify some data

        context.m_data = 1;

        // Perform some computation for an amount of time less than the cost

        std::this_thread::sleep_for(s_cost_duration - std::chrono::milliseconds(100));
    }
};

class task_2_t : public bratley::task_t<context_t, 1, 1, 5>
{
public:
    virtual void task(context_t &context) override
    {
        std::cout << "Executing task 2" << std::endl;

        // Modify some data

        context.m_data = 2;

        // Perform some computation for an amount of time less than the cost

        std::this_thread::sleep_for(s_cost_duration - std::chrono::milliseconds(100));
    }
};

class task_3_t : public bratley::task_t<context_t, 1, 2, 6>
{
public:
    virtual void task(context_t &context) override
    {
        std::cout << "Executing task 3" << std::endl;

        // Modify some data

        context.m_data = 3;

        // Perform some computation for an amount of time less than the cost

        std::this_thread::sleep_for(s_cost_duration - std::chrono::milliseconds(100));
    }
};

//class task_4_t : public bratley::task_t<context_t, 2, 1, 3> // From the exam
class task_4_t : public bratley::task_t<context_t, 0, 2, 4> // From the book
{
public:
    virtual void task(context_t &context) override
    {
        std::cout << "Executing task 4" << std::endl;

        // Modify some data

        context.m_data = 4;

        // Perform some computation for an amount of time less than the cost

        std::this_thread::sleep_for(s_cost_duration - std::chrono::milliseconds(100));
    }
};

int main(int argc, char **argv)
{
    auto schedules = bratley::schedule<task_1_t, task_2_t, task_3_t, task_4_t>();

    static_assert(std::tuple_size_v<decltype(schedules)> > 0, "No valid schedules");

    std::cout << "Number of valid schedules = " << std::tuple_size_v<decltype(schedules)> << std::endl;

    auto schedule = std::get<0>(schedules);

    context_t context;

    if (bratley::execute(context, schedule) == bratley::execute_status_t::SUCCESS) {
        std::cout << "SUCCESS" << std::endl;
    }
    else {
        std::cout << "FAILURE" << std::endl;
    }

    return 0;
}