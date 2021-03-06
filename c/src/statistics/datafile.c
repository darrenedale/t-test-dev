#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include "datafile.h"
#include "util.h"

int dataFileRangeItemCount(const DataFile *, int, int, int, int);
double dataFileRangeSum(const DataFile *, int, int, int, int, double);
double dataFileRangeMean(const DataFile *, int, int, int, int, double);
bool reloadDataFile(DataFile *, const char *);
void resetDataFile(DataFile *);

DataFile * newDataFile(const char * fileName)
{
    DataFile * dataFile = (DataFile *) malloc(sizeof(DataFile));
    dataFile->data = (void *) 0;
    dataFile->rowCount = 0;
    dataFile->columnCounts = (void *) 0;

    if (fileName) {
        reloadDataFile(dataFile, fileName);
    }

    return dataFile;
}

void freeDataFile(DataFile ** dataFile)
{
    resetDataFile(*dataFile);
    free(*dataFile);
    *dataFile = (void *) 0;
}

bool dataFileIsEmpty(const DataFile * dataFile)
{
    return 0 == dataFile->rowCount;
}

int dataFileRowCount(const DataFile * dataFile)
{
    return dataFile->rowCount;
}

int dataFileColumnCount(const DataFile * dataFile)
{
    if (!dataFile->columnCounts) {
        return 0;
    }

    return dataFile->columnCounts[0];
}

int dataFileItemCount(const DataFile * dataFile)
{
    return dataFileRangeItemCount(dataFile, 0, 0, dataFileRowCount(dataFile) - 1, dataFileColumnCount(dataFile) - 1);
}

int dataFileRowItemCount(const DataFile * dataFile, int row)
{
    assert(0 <= row && dataFile->rowCount > row);
    return dataFileRangeItemCount(dataFile, row, 0, row, dataFileColumnCount(dataFile) - 1);
}

int dataFileColumnItemCount(const DataFile * dataFile, int column)
{
    assert(0 <= column && dataFileColumnCount(dataFile) > column);
    return dataFileRangeItemCount(dataFile, 0, column, dataFileRowCount(dataFile) - 1, column);
}

double dataFileSum(const DataFile * dataFile, double power)
{
    return dataFileRangeSum(dataFile, 0, 0, dataFileRowCount(dataFile) - 1, dataFileColumnCount(dataFile) - 1, power);
}

double dataFileRowSum(const DataFile * dataFile, int row, double power)
{
    assert(0 <= row && dataFile->rowCount > row);
    return dataFileRangeSum(dataFile, row, 0, row, dataFileColumnCount(dataFile) - 1, power);
}

double dataFileColumnSum(const DataFile * dataFile, int column, double power)
{
    assert(0 <= column && dataFileColumnCount(dataFile) > column);
    return dataFileRangeSum(dataFile, 0, column, dataFileRowCount(dataFile) - 1, column, power);
}

double dataFileMean(const DataFile * dataFile, double meanNumber)
{
    return dataFileRangeMean(dataFile, 0, 0, dataFileRowCount(dataFile) - 1, dataFileColumnCount(dataFile) - 1, meanNumber);
}

double dataFileRowMean(const DataFile * dataFile, int row, double meanNumber)
{
    assert(0 <= row && dataFile->rowCount > row);
    return dataFileRangeMean(dataFile, row, 0, row, dataFileColumnCount(dataFile) - 1, meanNumber);
}

double dataFileColumnMean(const DataFile * dataFile, int column, double meanNumber)
{
    assert(0 <= column && dataFileColumnCount(dataFile) > column);
    return dataFileRangeMean(dataFile, 0, column, dataFileRowCount(dataFile) - 1, column, meanNumber);
}

double dataFileItem(const DataFile * dataFile, int row, int column)
{
    assert(0 <= row && row < dataFile->rowCount);
    assert(0 <= column && column < dataFileColumnCount(dataFile));
    return dataFile->data[row][column];
}

/*
 * Private functions.
 */
int dataFileRangeItemCount(const DataFile * dataFile, int firstRow, int firstColumn, int lastRow, int lastColumn)
{
    int count = 0;

    for (int row = firstRow; row <= lastRow; ++row) {
        int rowLastColumn = min(lastColumn, dataFile->columnCounts[row] - 1);

        for (int col = firstColumn; col <= rowLastColumn; ++col) {
            if (!isnan(dataFile->data[row][col])) {
                ++count;
            }
        }
    }

    return count;
}

double dataFileRangeSum(const DataFile * dataFile, int firstRow, int firstColumn, int lastRow, int lastColumn, double power)
{
    double sum = 0.0;

    for (int row = firstRow; row <= lastRow; ++row) {
        int rowLastColumn = min(lastColumn, dataFile->columnCounts[row] - 1);

        for (int col = firstColumn; col <= rowLastColumn; ++col) {
            if (!isnan(dataFile->data[row][col])) {
                sum += pow(dataFile->data[row][col], power);
            }
        }
    }

    return sum;
}

double dataFileRangeMean(const DataFile * dataFile, int firstRow, int firstColumn, int lastRow, int lastColumn, double meanNumber)
{
    double sum = 0.0;
    int n = 0;

    for (int row = firstRow; row <= lastRow; ++row) {
        int rowLastColumn = min(lastColumn, dataFile->columnCounts[row] - 1);

        for (int col = firstColumn; col <= rowLastColumn; ++col) {
            if (!isnan(dataFile->data[row][col])) {
                sum += pow(dataFile->data[row][col], meanNumber);
                ++n;
            }
        }
    }

    return pow(sum / n, 1.0 / meanNumber);
}

void resetDataFile(DataFile * dataFile)
{
    if (dataFile->data) {
        for (int row = 0; row < dataFile->rowCount; ++row) {
            free(dataFile->data[row]);
            dataFile->data[row] = (void *) 0;
        }

        free(dataFile->data);
        dataFile->data = (void *) 0;
    }

    if (dataFile->columnCounts) {
        free(dataFile->columnCounts);
        dataFile->columnCounts = (void *) 0;
    }

    dataFile->rowCount = 0;
}

void parseLineIntoDataFile(DataFile * dataFile, char * line)
{
    int col = 0;

    if (!dataFile->data) {
        dataFile->data = (double **) malloc(sizeof(double *));
    } else {
        dataFile->data = (double **) realloc(dataFile->data, (dataFile->rowCount + 1) * sizeof(double *));
    }

    int capacity = 10;
    dataFile->data[dataFile->rowCount] = malloc(sizeof(double) * capacity);
    char * start = line;
    char * end;

    while (*start) {
        end = start;

        while (*end && ',' != *end) {
            ++end;
        }

        char delimiter = *end;
        char * firstUnparsed;
        *end = 0;
        double value = strtod(start, &firstUnparsed);

        // check whether the parsed value consumed all the non-whitespace in the cell
        while (*firstUnparsed) {
            if (!isspace(*firstUnparsed)) {
                // invalid value
                value = NAN;
                break;
            }

            ++firstUnparsed;
        }

        if (col == capacity) {
            capacity *= 1.5;
            dataFile->data[dataFile->rowCount] = realloc(dataFile->data[dataFile->rowCount], sizeof(double) * capacity);
        }

        dataFile->data[dataFile->rowCount][col] = value;
        ++col;

        if (!delimiter) {
            // EOL - exit parse loop
            break;
        }

        start = end + 1;
    }

    // add the column count for the row
    if (!dataFile->columnCounts) {
        dataFile->columnCounts = (int *) malloc(sizeof(int));
    } else {
        dataFile->columnCounts = (int *) realloc(dataFile->columnCounts, (dataFile->rowCount + 1) * sizeof(int));
    }

    dataFile->columnCounts[dataFile->rowCount] = col;
    ++dataFile->rowCount;
}

bool reloadDataFile(DataFile * dataFile, const char * fileName)
{
    resetDataFile(dataFile);
    FILE * inFile = fopen(fileName, "r");

    if (!inFile) {
        return false;
    }

    {
        int bufferSize = 1024;
        char * buffer = (char *) malloc(bufferSize);
        char * eol = buffer;

        while (!feof(inFile)) {
            if (eol == buffer + bufferSize) {
                // reallocate buffer
                int previousBufferSize = bufferSize;
                bufferSize *= 1.5;
                buffer = realloc(buffer, bufferSize);
                eol = buffer + previousBufferSize;
            }

            *eol = (char) fgetc(inFile);

            if (EOF == *eol || 10 == *eol) {
                // found a line
                *eol = 0;
                parseLineIntoDataFile(dataFile, buffer);
                eol = buffer;
            } else {
                ++eol;
            }
        }

        free (buffer);
    }

    fclose(inFile);
    return true;
}
