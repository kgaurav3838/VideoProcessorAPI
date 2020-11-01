// MyProject1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

#include <stdio.h>
#include <intrin.h>

#pragma comment (lib, "ole32.lib")
#pragma comment (lib, "mf.lib")
#pragma comment (lib, "mfplat.lib")
#pragma comment (lib, "mfuuid.lib")
#pragma comment (lib, "mfreadwrite.lib")

#define ASSERT(x) do { if (!(x)) __debugbreak(); } while (0)
#define CHECK(hr) ASSERT(SUCCEEDED(hr))

int main(int argc, char* argv[])
{
    HRESULT hr;

    hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    CHECK(hr);

    hr = MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
    CHECK(hr);

    if (argc == 1) // enumerate available devices
    {
        UINT32 count;
        IMFActivate** devices;

        {
            IMFAttributes* attr;

            hr = MFCreateAttributes(&attr, 1);
            CHECK(hr);

            hr = IMFAttributes_SetGUID(attr, &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
            CHECK(hr);

            hr = MFEnumDeviceSources(attr, &devices, &count);
            CHECK(hr);

            IMFAttributes_Release(attr);
        }

        printf("Detected %u devices:\n", count);

        for (UINT32 i = 0; i < count; i++)
        {
            UINT32 length;
            LPWSTR name;
            LPWSTR symlink;

            hr = IMFActivate_GetAllocatedString(devices[i], &MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &name, &length);
            CHECK(hr);

            hr = IMFActivate_GetAllocatedString(devices[i], &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, &symlink, &length);
            CHECK(hr);

            printf("%S = %S\n", name, symlink);

            CoTaskMemFree(name);
            CoTaskMemFree(symlink);

            IMFActivate_Release(devices[i]);
        }

        CoTaskMemFree(devices);
    }
    else // create device from name
    {
        IMFMediaSource* device;
        {
            IMFAttributes* attr;

            hr = MFCreateAttributes(&attr, 2);
            CHECK(hr);

            hr = IMFAttributes_SetGUID(attr, &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
            CHECK(hr);

            WCHAR name[1024];
            MultiByteToWideChar(CP_UTF8, 0, argv[1], -1, name, 1024);

            hr = IMFAttributes_SetString(attr, &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, name);
            CHECK(hr);

            hr = MFCreateDeviceSource(attr, &device);
            CHECK(hr);

            IMFAttributes_Release(attr);
        }

        IMFSourceReader* reader;

        hr = MFCreateSourceReaderFromMediaSource(device, NULL, &reader);
        CHECK(hr);

        IMFMediaSource_Release(device);

        // this assumes camera can provide mjpeg output
        // typically webcams provide YUV2 format, you'll need to convert it to
        // RGB yourself or with help of IMFTransform
        // you can enumerate all supported types with IMFSourceReader_GetNativeMediaType
        {
            IMFMediaType* type;

            hr = MFCreateMediaType(&type);
            CHECK(hr);

            hr = IMFMediaType_SetGUID(type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
            CHECK(hr);

            hr = IMFMediaType_SetGUID(type, &MF_MT_SUBTYPE, &MFVideoFormat_MJPG);
            CHECK(hr);

            // you can also set desired width/height here

            hr = IMFSourceReader_SetCurrentMediaType(reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, type);
            CHECK(hr);

            IMFMediaType_Release(type);
        }

        UINT32 width;
        UINT32 height;

        // get width/height
        {
            IMFMediaType* type;

            hr = IMFSourceReader_GetCurrentMediaType(reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM, &type);
            CHECK(hr);

            UINT64 tmp;

            hr = IMFMediaType_GetUINT64(type, &MF_MT_FRAME_SIZE, &tmp);
            CHECK(hr);

            width = (UINT32)(tmp >> 32);
            height = (UINT32)(tmp);

            IMFMediaType_Release(type);
        }

        printf("Size = %ux%u\n", width, height);

        // read one frame and save it to file
        {
            IMFSample* sample;

            DWORD stream;
            DWORD flags;
            LONGLONG timestamp;

            for (;;)
            {
                // this is reading in syncronous blocking mode, MF supports also async calls
                hr = IMFSourceReader_ReadSample(reader, MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &stream, &flags, &timestamp, &sample);
                CHECK(hr);

                if (flags & MF_SOURCE_READERF_STREAMTICK)
                {
                    continue;
                }

                break;
            }

            {
                IMFMediaBuffer* buffer;

                hr = IMFSample_ConvertToContiguousBuffer(sample, &buffer);
                CHECK(hr);

                BYTE* data;
                DWORD size;

                hr = IMFMediaBuffer_Lock(buffer, &data, NULL, &size);
                CHECK(hr);

                {
                    HANDLE h = CreateFileA("image.jpg", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                    ASSERT(h != INVALID_HANDLE_VALUE);

                    DWORD written;
                    BOOL ok = WriteFile(h, data, size, &written, NULL);
                    ASSERT(ok && written == size);

                    CloseHandle(h);
                }

                IMFMediaBuffer_Unlock(buffer);

                IMFMediaBuffer_Release(buffer);
            }

            IMFSample_Release(sample);
        }

        IMFSourceReader_Release(reader);
    }

    MFShutdown();

    CoUninitialize();
}