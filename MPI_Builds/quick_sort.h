// from https://en.wikipedia.org/wiki/Quicksort#:~:text=Quicksort%20is%20a%20divide%2Dand,or%20greater%20than%20the%20pivot. 
#include <algorithm>
#include <iostream>

// Divides array into two partitions
int partition(int* &A, int lo, int hi) { 
  int pivot = A[hi]; // Choose the last element as the pivot

  // Temporary pivot index
  int i = lo;

  for (int j = lo; j <= hi - 1; j++) { 
    // If the current element is less than or equal to the pivot
    if (A[j] <= pivot) { 
      // Swap the current element with the element at the temporary pivot index
      std::swap(A[i], A[j]);
      // Move the temporary pivot index forward
      i = i + 1;
    }
  }

  // Swap the pivot with the last element
  std::swap(A[i],A[hi]);

  return i; // the pivot index
}

// Sorts a (portion of an) array, divides it into partitions, then sorts those
void quicksort(int* &A, int lo, int hi) { 
  // Ensure indices are in correct order
  if (lo >= hi || lo < 0) { 
    return;
  }
    
  // Partition array and get the pivot index
  int p = partition(A, lo, hi);
      
  // Sort the two partitions
  quicksort(A, lo, p - 1); // Left side of pivot
  quicksort(A, p + 1, hi); // Right side of pivot
}


// int main() {
//     int* array1 = new int[4];
//     for (int i=0; i<4; i++) {
//         array1[i] = i;
//     }
//     quicksort(array1,0,3);

//     for (int i=0; i<4; i++) {
//         std::cout << array1[i];
//     }

//     delete array1;

//     int* array2 = new int[4];
//     for (int i=0; i<4; i++) {
//         array2[i] = 4-i;
//     }
//     quicksort(array2,0,3);

//     for (int i=0; i<4; i++) {
//         std::cout << array2[i];
//     }
//     delete array2;


//     int* array3 = new int[10];
//     for (int i=0; i<5; i++) {
//         array3[i] = i;
//     }
//     for (int i=5; i<10; i++) {
//         array3[i] = 9-i;
//     }
//     quicksort(array3,0,10);

//     std::cout << std::endl;

//     for (int i=0; i<10; i++) {
//         std::cout << array3[i];
//     }
//     delete array3;
// }