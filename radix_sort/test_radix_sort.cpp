#include <iostream>
#include <vector>
#include <numeric>
#include <random>
#include <climits>
#include <algorithm>
#include <mpi.h>

enum sort_type {
    sorted,
    ran,
    one_percent,
    reverse_sort 
};

#define MASTER 0

void data_init(const int taskid, const int numtasks, const int n_each, std::vector<int>& data, sort_type sort_type);
void radix_sort(std::vector<int>& data);

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int taskid, numtasks;
    MPI_Comm_rank(MPI_COMM_WORLD, &taskid);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

    int n_each = 1 << 21; 
    std::vector<int> data;

    std::vector<sort_type> sort_types = {sorted, ran, one_percent, reverse_sort};

    for (auto st : sort_types) {
        data_init(taskid, numtasks, n_each, data, st);

        // debug
        if (taskid == MASTER) {
            std::cout << "Before sorting, sort type: " << st << std::endl;
            for (const auto& num : data) {
                std::cout << num << " ";
            }
            std::cout << std::endl;
        }

        
        radix_sort(data);

        // debug
        if (taskid == MASTER) {
            std::cout << "After sorting, sort type: " << st << std::endl;
            for (const auto& num : data) {
                std::cout << num << " ";
            }
            std::cout << std::endl;
        }

        
        MPI_Barrier(MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return 0;
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
        // Initialize data as sorted
        std::iota(data.begin(), data.end(), taskid * n_each);

        int perturb_count = static_cast<int>(n_each * 0.01);
        if (perturb_count == 0) 
            perturb_count = 1;

        // perturb 1% of data
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> index_dist(0, n_each - 1);
        std::uniform_int_distribution<> value_dist(INT32_MIN, INT32_MAX);

        for (int i = 0; i < perturb_count; ++i) {
            int idx = index_dist(gen);
            data[idx] = value_dist(gen);
        }
    }

    std::cout << "Task " << taskid << " initialized data for sort type " << sort_type << ": ";
    for (const auto& num : data) {
        std::cout << num << " ";
    }
    std::cout << std::endl;
}

void counting_sort(std::vector<int>& data, int exp) {
    int n = data.size();
    std::vector<int> output(n);
    int count[10] = {0};

    // debug
    std::cout << "Counting sort on exp: " << exp << std::endl;
    std::cout << "Data: ";
    for (const auto& num : data) {
        std::cout << num << " ";
    }
    std::cout << std::endl;

    for (int i = 0; i < n; i++) {
        int digit = (data[i] / exp) % 10;
        if (digit < 0) digit += 10; 
        count[digit]++;
    }

    std::cout << "Count array: ";
    for (int i = 0; i < 10; i++) {
        std::cout << count[i] << " ";
    }
    std::cout << std::endl;

    for (int i = 1; i < 10; i++) {
        count[i] += count[i - 1];
    }

    std::cout << "Cumulative count array: ";
    for (int i = 0; i < 10; i++) {
        std::cout << count[i] << " ";
    }
    std::cout << std::endl;

    for (int i = n - 1; i >= 0; i--) {
        int digit = (data[i] / exp) % 10;
        if (digit < 0) digit += 10; 
        output[count[digit] - 1] = data[i];
        count[digit]--;
    }

    std::cout << "Output array: ";
    for (const auto& num : output) {
        std::cout << num << " ";
    }
    std::cout << std::endl;

    for (int i = 0; i < n; i++) {
        data[i] = output[i];
    }
}

void radix_sort(std::vector<int>& data) {
    if (data.empty()) return; 

    int max_val = *std::max_element(data.begin(), data.end());
    int min_val = *std::min_element(data.begin(), data.end());

    // make non negative
    int shift = (min_val < 0) ? -min_val : 0;
    for (auto& num : data) {
        num += shift;
    }

    for (int exp = 1; (max_val + shift) / exp > 0; exp *= 10) {
        counting_sort(data, exp);
    }

    // shift back  to original valus
    for (auto& num : data) {
        num -= shift;
    }
}