// Project2.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <algorithm>
#include <cmath>
#include <stdint.h>
#include <string>
#include "stl_reader.h"

struct Vertex {
    float x, y, z;
};

// Define MME header struct
#pragma pack(push, 1)
struct MMEHeader {
    char attribute[8];
    unsigned int nObjects;
    unsigned int reserved;
};
#pragma pack(pop)

// Define object allocation table entry struct
#pragma pack(push, 1)
struct AllocationEntry {
    unsigned int offset;
    unsigned int localNumber;
    unsigned int objectType;
    unsigned int objectSize;
};
#pragma pack(pop)

struct Triangle {
    Vertex normal;
    Vertex vertices[3];
};

inline ULONG endian_swap(ULONG x)
{
    return (x >> 24) |
        ((x << 8) & 0x00FF0000) |
        ((x >> 8) & 0x0000FF00) |
        (x << 24);
}

//  ErrorMessage support function.
//  Retrieves the system error message for the GetLastError() code.
//  Note: caller must use LocalFree() on the returned LPCTSTR buffer.
LPCTSTR ErrorMessage(DWORD error)
{
    LPVOID lpMsgBuf;

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER
        | FORMAT_MESSAGE_FROM_SYSTEM
        | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf,
        0,
        NULL);

    return((LPCTSTR)lpMsgBuf);
}
//  PrintError support function.
//  Simple wrapper function for error output.
void PrintError(LPCTSTR errDesc)
{
    LPCTSTR errMsg = ErrorMessage(GetLastError());
   // _tprintf(TEXT("\n** ERROR ** %s: %s\n"), errDesc, errMsg);
    LocalFree((LPVOID)errMsg);
}

void read_ascii_stl_file(const std::string& file_path, std::vector<Triangle>& triangles) {
    std::vector<float> coords, normals;
    std::vector<unsigned int> tris, solids;

    try {
        Triangle triangle;
        stl_reader::ReadStlFile(file_path.c_str(), coords, normals, tris, solids);
        const size_t numTris = tris.size() / 3;
        for (size_t itri = 0; itri < numTris; ++itri) {
            std::cout << "coordinates of triangle " << itri << ": ";
            for (size_t icorner = 0; icorner < 3; ++icorner) {
                float* c = &coords[3 * tris[3 * itri + icorner]];
                triangle.vertices[icorner].x = c[0];
                triangle.vertices[icorner].y = c[1];
                triangle.vertices[icorner].z = c[2];
                //std::cout << "(" << c[0] << ", " << c[1] << ", " << c[2] << ") ";
            }
            // std::cout << std::endl;
             // Add the triangle to the list of triangles
            triangles.push_back(triangle);
            //float* n = &normals[3 * itri];
           // std::cout << "normal of triangle " << itri << ": "
                //<< "(" << n[0] << ", " << n[1] << ", " << n[2] << ")\n";
        }
    }
    catch (std::exception& e) {
        std::cout << e.what() << std::endl;
    }
}

void write_mme_file(const std::wstring& file_path, const std::vector<Triangle>& triangles) 
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    
    //  Opens the existing file. 
    hFile = CreateFile(file_path.c_str(),               // file name 
        GENERIC_READ | GENERIC_WRITE,          // open for reading 
        FILE_SHARE_READ | FILE_SHARE_WRITE,                     // do not share 
        NULL,                  // default security 
        CREATE_ALWAYS,         // existing file only 
        0, // normal file 
        NULL);                 // no template 
    if (hFile == INVALID_HANDLE_VALUE)
    {
        PrintError(TEXT("First CreateFile failed"));
        return;
    }

    LARGE_INTEGER	fileOffset;
    DWORD dwNumBytes = 0;
    BOOL res = FALSE;
    // Write the MME header
    ULONG iTriCount = triangles.size();
    MMEHeader header;
    memcpy(&header.attribute[0], "TITLE 02", 8);
    header.nObjects = iTriCount;
    header.reserved = 0;
    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
    res = WriteFile(hFile, &header, sizeof(MMEHeader), &dwNumBytes, NULL);
    DWORD lasterror = GetLastError();
    //fileOffset.QuadPart = 1LL * iBlock * dwBlockSize;

    // Write the object allocation table entry
    DWORD batSize = iTriCount * 16;
    AllocationEntry *batEntry = new AllocationEntry[iTriCount];
    memset(batEntry, 0, batSize);

    for (int i = 0; i < triangles.size(); ++i) {
        AllocationEntry entry;
        entry.offset = batSize + (i * 12 * 3);
        entry.localNumber = i + 1;
        entry.objectType = 0x3000; // Object type identifier for STL data
        entry.objectSize = 12 * 3;
        memcpy(&batEntry[i], &entry, sizeof(AllocationEntry));
    }
    res = WriteFile(hFile, &batEntry, batSize, &dwNumBytes, NULL);

    // Write the object data
    for (int i = 0; i < triangles.size(); ++i) {
        const Triangle& triangle = triangles[i];
        // Write the vertices
        for (int j = 0; j < 3; ++j) {
            const Vertex& vertex = triangle.vertices[j];

            res = WriteFile(hFile, &vertex, sizeof(Vertex), &dwNumBytes, NULL);

            /* mme_file.write(reinterpret_cast<const char*>(&vertex.x), sizeof(float));
             mme_file.write(reinterpret_cast<const char*>(&vertex.y), sizeof(float));
             mme_file.write(reinterpret_cast<const char*>(&vertex.z), sizeof(float));*/
        }
    }

    CloseHandle(hFile);
    //// Open the MME file for writing
    //std::ofstream mme_file(file_path, std::ios::binary);

    //// Write the object data
    //for (int i = 0; i < triangles.size(); ++i) {
    //    const Triangle& triangle = triangles[i];
    //    // Write the vertices
    //    for (int j = 0; j < 3; ++j) {
    //        const Vertex& vertex = triangle.vertices[j];

    //        mme_file.write(reinterpret_cast<const char*>(&vertex), sizeof(Vertex));

    //        /* mme_file.write(reinterpret_cast<const char*>(&vertex.x), sizeof(float));
    //         mme_file.write(reinterpret_cast<const char*>(&vertex.y), sizeof(float));
    //         mme_file.write(reinterpret_cast<const char*>(&vertex.z), sizeof(float));*/
    //    }
    //}

    //mme_file.close();
}

int main() {
    std::vector<Triangle> triangles;
    std::string stl_file_path = "0333.stl";
    std::wstring mme_file_path = L"test.mme";

    //try {
    //    // Read the ASCII STL file
        read_ascii_stl_file(stl_file_path, triangles);

    //    // Write the MME file
        write_mme_file(mme_file_path, triangles);

    //    std::cout << "MME file saved successfully!" << std::endl;
    //}
    //catch (std::exception& ex) {
    //    std::cerr << "Error: " << ex.what() << std::endl;
    //}

    return 0;
}



// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
