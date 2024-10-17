#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <climits>
#include <mpi.h> 

enum sort_type {
    sorted,
    ran,
    one_percent,
    reverse_sort
};

#define MASTER 0

void data_init(const int taskid, const int numtasks, const int n_each, std::vector<int>& data, sort_type sort_type);

void counting_sort(std::vector<int>& data, int exp) {
    int n = data.size();
    std::vector<int> output(n);
    int count[10] = {0};

    for (int i = 0; i < n; i++) {
        int digit = (data[i] / exp) % 10;
        count[digit]++;
    }

    for (int i = 1; i < 10; i++) {
        count[i] += count[i - 1];
    }

    for (int i = n - 1; i >= 0; i--) {
        int digit = (data[i] / exp) % 10;
        output[count[digit] - 1] = data[i];
        count[digit]--;
    }

    for (int i = 0; i < n; i++) {
        data[i] = output[i];
    }
}

void radix_sort(std::vector<int>& data) {
    int max_val = *std::max_element(data.begin(), data.end());

    for (int exp = 1; max_val / exp > 0; exp *= 10) {
        counting_sort(data, exp);
    }
}

void data_init(const int taskid, const int numtasks, const int n_each, std::vector<int>& data, sort_type sort_type) {
    data.resize(n_each);
    if (sort_type == sorted) {
        std::iota(data.begin(), data.end(), taskid * n_each);
    } else if (sort_type == reverse_sort) {
        std::iota(data.begin(), data.end(), (numtasks - taskid - 1) * n_each);
        std::reverse(data.begin(), data.end());
    } else if (sort_type == ran) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(INT32_MIN, INT32_MAX);

        std::generate(data.begin(), data.end(), [&]() { return dist(gen); });
    } else if (sort_type == one_percent) {
        
        std::iota(data.begin(), data.end(), taskid * n_each);

        int perturb_count = static_cast<int>(n_each * 0.01);
        if (perturb_count == 0) perturb_count = 1; 

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> index_dist(0, n_each - 1);
        std::uniform_int_distribution<> value_dist(INT32_MIN, INT32_MAX);

        for (int i = 0; i < perturb_count; ++i) {
            int idx = index_dist(gen);
            data[idx] = value_dist(gen);
        }

        int partner_task;
        if (numtasks % 2 == 0) {
            // for even number of tasks, pair taskid with taskid ^ 1
            partner_task = taskid ^ 1;
        } else {
            // for odd number of tasks, pair taskid with (taskid + 1) % numtasks
            partner_task = (taskid + 1) % numtasks;
        }

        // make sure partner_task is within bounds
        if (partner_task >= numtasks) {
            partner_task = taskid; 
        }

        if (partner_task != taskid) {
            std::vector<int> recv_data(perturb_count);

            // exchange data with partner
            MPI_Sendrecv(&data[0], perturb_count, MPI_INT, partner_task, 0,
                         &recv_data[0], perturb_count, MPI_INT, partner_task, 0,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            // swap first perturb_count elements with received data
            for (int i = 0; i < perturb_count; ++i) {
                data[i] = recv_data[i];
            }
        }
    }
}