#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>
#include <random>
#include <chrono>
#include <string>
#include <functional>

using namespace std;

// Function to generate a random number in a given range
int getRandomNumber(int min, int max) {
    static random_device rd;
    static mt19937 gen(rd());
    uniform_int_distribution<int> dist(min, max);
    return dist(gen);
}

// Function to generate a sorted array
vector<int> generateSortedArray(int size) {
    vector<int> arr(size);
    for (int i = 0; i < size; ++i) {
        arr[i] = i + 1;
    }
    return arr;
}

// Function to generate a reverse sorted array
vector<int> generateReverseSortedArray(int size) {
    vector<int> arr(size);
    for (int i = 0; i < size; ++i) {
        arr[i] = size - i;
    }
    return arr;
}

// Function to generate a random array
vector<int> generateRandomArray(int size) {
    vector<int> arr(size);
    for (int i = 0; i < size; ++i) {
        arr[i] = getRandomNumber(1, size);
    }
    return arr;
}

// Function to generate a 1% perturbed array
vector<int> generatePerturbedArray(int size) {
    vector<int> arr = generateSortedArray(size);
    int numPerturbed = size / 100;  // 1% of the elements
    for (int i = 0; i < numPerturbed; ++i) {
        int idx1 = getRandomNumber(0, size - 1);
        int idx2 = getRandomNumber(0, size - 1);
        swap(arr[idx1], arr[idx2]);
    }
    return arr;
}

// Function to write array to a file
void writeArrayToFile(const vector<int>& arr, const string& filename) {
    ofstream outFile(filename);
    if (outFile.is_open()) {
        for (const int& num : arr) {
            outFile << num << " ";
        }
        outFile.close();
        cout << "Data written to: " << filename << endl;
    } else {
        cerr << "Error opening file: " << filename << endl;
    }
}

int main() {
    // Define input sizes and map the options to their powers of two
    vector<pair<string, int>> inputSizes = {
        {"2^16", 65536},
        {"2^18", 262144},
        {"2^20", 1048576},
        {"2^22", 4194304},
        {"2^24", 16777216},
        {"2^26", 67108864},
        {"2^28", 268435456}
    };

    // Define input types and corresponding generator functions
    vector<pair<string, function<vector<int>(int)>>> inputTypes = {
        {"Sorted", generateSortedArray},
        {"Random", generateRandomArray},
        {"Reverse Sorted", generateReverseSortedArray},
        {"1% Perturbed", generatePerturbedArray}
    };

    // Allow the user to select input size
    cout << "Select an input size from the following options:" << endl;
    for (size_t i = 0; i < inputSizes.size(); ++i) {
        cout << i + 1 << ". " << inputSizes[i].first << endl;
    }

    int sizeChoice;
    cout << "Enter the number corresponding to your choice: ";
    cin >> sizeChoice;

    // Validate input size choice
    if (sizeChoice < 1 || sizeChoice > inputSizes.size()) {
        cerr << "Invalid choice for input size. Exiting program." << endl;
        return 1;
    }
    int selectedSize = inputSizes[sizeChoice - 1].second;

    // Allow the user to select input type
    cout << "\nSelect an array type from the following options:" << endl;
    for (size_t i = 0; i < inputTypes.size(); ++i) {
        cout << i + 1 << ". " << inputTypes[i].first << endl;
    }

    int typeChoice;
    cout << "Enter the number corresponding to your choice: ";
    cin >> typeChoice;

    // Validate input type choice
    if (typeChoice < 1 || typeChoice > inputTypes.size()) {
        cerr << "Invalid choice for input type. Exiting program." << endl;
        return 1;
    }
    string selectedType = inputTypes[typeChoice - 1].first;
    auto generator = inputTypes[typeChoice - 1].second;

    // Generate the array based on the user's choices
    vector<int> array = generator(selectedSize);

    // Create a filename based on input size and type
    string filename = "input_" + to_string(selectedSize) + "_" + selectedType + ".txt";

    // Write the array to the file
    writeArrayToFile(array, filename);

    return 0;
}
