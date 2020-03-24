// Brandon Lewien
// Lab 0

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define SIZE_8_TYPE     256
#define MAX_ARRAY_SIZE 1024

static void mergeSort(int length, int middle, int array[])
{
    int left[(length+1)/2], right[(length+1)/2];
    int numLeftElements, numRightElements;
    int leftIterator  = 0;
    int rightIterator = 0; 
    int mainIterator  = 0; // To merge back
    if (length > 1)
    {
        for(numLeftElements = 0; numLeftElements < middle; ++numLeftElements)
        {
            left[numLeftElements] = array[numLeftElements];
        }
        for(numRightElements = 0; numRightElements < (length - numLeftElements); ++numRightElements)
        {
            right[numRightElements] = array[numRightElements+middle];
        }
        mergeSort(numLeftElements, numLeftElements/2, left);
        mergeSort(numRightElements, numRightElements/2, right);

        while (leftIterator < numLeftElements && rightIterator < numRightElements)
        {
            if (left[leftIterator] <= right[rightIterator])
            {
                array[mainIterator]=left[leftIterator];
                leftIterator++;
            }
            else 
            {
                array[mainIterator]=right[rightIterator];
                rightIterator++;
            }
            mainIterator++;
        }
        // Add the leftovers back for both left and right
        while (leftIterator < numLeftElements)
        {
            array[mainIterator]=left[leftIterator];
            leftIterator++;
            mainIterator++;
        }
        while (rightIterator < numRightElements)
        {
            array[mainIterator]=right[rightIterator];
            rightIterator++;
            mainIterator++;
        }
    }
}

int main(int argc, char* argv[]) 
{
    (void)argc;
    int rawFinal[MAX_ARRAY_SIZE];
    int incrementer = 0;
    char integer[SIZE_8_TYPE];

    if (strcmp(argv[1], "--name") == 0)
    {
        printf("Brandon Lewien\n");
        return 24;
    }

    FILE * filePointer;
    filePointer = fopen(argv[1],"r");
    if (filePointer == NULL) 
    {
        return -1;
    }

    while (fgets(integer, sizeof(integer), filePointer) != NULL) 
    {
        rawFinal[incrementer] = atoi(integer);
        ++incrementer;
    }
    fclose(filePointer);
    mergeSort(incrementer, incrementer/2, rawFinal);
    if (argv[2] != NULL)
    {
        if (strcmp(argv[2], "-o") == 0)
        {
            if (argv[3] == NULL)
            {
                printf("No Third Input!\n");
                return -1;
            }

            filePointer = fopen(argv[3], "w");
            if (filePointer == NULL) 
            {
                return -1;
            }
            for (int i = 0; i < incrementer; ++i)
            {
                fprintf(filePointer, "%d\n", rawFinal[i]);
            }
            fclose(filePointer);
            return 0;
        }
    }
    for (int i = 0; i < incrementer; ++i)
    {
        printf("%d ", rawFinal[i]); 
    } 
    printf("\n"); 

    return 0;
}