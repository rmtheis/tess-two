#include <stdio.h>
#include "allheaders.h"
#include "pix.h"
#ifdef USE_OPENCL
#include "tiff.h"
#include "tiffio.h"
#endif
#include "tprintf.h"

// including CL/cl.h doesn't occur until USE_OPENCL defined below

// platform preprocessor commands
#if defined( WIN32 ) || defined( __WIN32__ ) || defined( _WIN32 ) || defined( __CYGWIN32__ ) || defined( __MINGW32__ )
#define ON_WINDOWS 1
#define ON_LINUX   0
#define ON_APPLE   0
#define ON_OTHER   0
#define IF_WINDOWS(X) X
#define IF_LINUX(X)
#define IF_APPLE(X)
#define IF_OTHER(X)
#define NOT_WINDOWS(X)
#elif defined( __linux__ )
#define ON_WINDOWS 0
#define ON_LINUX   1
#define ON_APPLE   0
#define ON_OTHER   0
#define IF_WINDOWS(X)
#define IF_LINUX(X) X
#define IF_APPLE(X)
#define IF_OTHER(X)
#define NOT_WINDOWS(X) X
#elif defined( __APPLE__ )
#define ON_WINDOWS 0
#define ON_LINUX   0
#define ON_APPLE   1
#define ON_OTHER   0
#define IF_WINDOWS(X)
#define IF_LINUX(X)
#define IF_APPLE(X) X
#define IF_OTHER(X)
#define NOT_WINDOWS(X) X
#else
#define ON_WINDOWS 0
#define ON_LINUX   0
#define ON_APPLE   0
#define ON_OTHER   1
#define IF_WINDOWS(X)
#define IF_LINUX(X)
#define IF_APPLE(X)
#define IF_OTHER(X) X
#define NOT_WINDOWS(X) X
#endif

#if ON_LINUX
#include <time.h>
#endif

#if ON_APPLE
#include <mach/clock.h>
#include <mach/mach.h>
#define CLOCK_MONOTONIC SYSTEM_CLOCK
#define clock_gettime clock_get_time
#endif

/************************************************************************************
 * enable/disable reporting of performance
 * PERF_REPORT_LEVEL
 * 0 - no reporting
 * 1 - no reporting
 * 2 - report total function call time for functions we're tracking
 * 3 - optionally report breakdown of function calls (kernel launch, kernel time, data copies)
 ************************************************************************************/
#define PERF_COUNT_VERBOSE 1
#define PERF_COUNT_REPORT_STR "[%36s], %24s, %11.6f\n"


#if ON_WINDOWS

#if PERF_COUNT_VERBOSE >= 2
#define PERF_COUNT_START(FUNCT_NAME) \
    char *funct_name = FUNCT_NAME; \
    double elapsed_time_sec; \
    LARGE_INTEGER freq, time_funct_start, time_funct_end, time_sub_start, time_sub_end; \
    QueryPerformanceFrequency(&freq); \
    QueryPerformanceCounter(&time_funct_start); \
    time_sub_start = time_funct_start; \
    time_sub_end = time_funct_start;

#define PERF_COUNT_END \
    QueryPerformanceCounter(&time_funct_end); \
    elapsed_time_sec = (time_funct_end.QuadPart-time_funct_start.QuadPart)/(double)(freq.QuadPart); \
    tprintf(PERF_COUNT_REPORT_STR, funct_name, "total", elapsed_time_sec);
#else
#define PERF_COUNT_START(FUNCT_NAME)
#define PERF_COUNT_END
#endif

#if PERF_COUNT_VERBOSE >= 3
#define PERF_COUNT_SUB(SUB) \
    QueryPerformanceCounter(&time_sub_end); \
    elapsed_time_sec = (time_sub_end.QuadPart-time_sub_start.QuadPart)/(double)(freq.QuadPart); \
    tprintf(PERF_COUNT_REPORT_STR, funct_name, SUB, elapsed_time_sec); \
    time_sub_start = time_sub_end;
#else
#define PERF_COUNT_SUB(SUB)
#endif


// not on windows
#else

#if PERF_COUNT_VERBOSE >= 2
#define PERF_COUNT_START(FUNCT_NAME) \
    char *funct_name = FUNCT_NAME; \
    double elapsed_time_sec; \
    timespec time_funct_start, time_funct_end, time_sub_start, time_sub_end; \
    clock_gettime( CLOCK_MONOTONIC, &time_funct_start ); \
    time_sub_start = time_funct_start; \
    time_sub_end = time_funct_start;

#define PERF_COUNT_END \
    clock_gettime( CLOCK_MONOTONIC, &time_funct_end ); \
    elapsed_time_sec = (time_funct_end.tv_sec - time_funct_start.tv_sec)*1.0 + (time_funct_end.tv_nsec - time_funct_start.tv_nsec)/1000000000.0; \
    tprintf(PERF_COUNT_REPORT_STR, funct_name, "total", elapsed_time_sec);
#else
#define PERF_COUNT_START(FUNCT_NAME)
#define PERF_COUNT_END
#endif

#if PERF_COUNT_VERBOSE >= 3
#define PERF_COUNT_SUB(SUB) \
    clock_gettime( CLOCK_MONOTONIC, &time_sub_end ); \
    elapsed_time_sec = (time_sub_end.tv_sec - time_sub_start.tv_sec)*1.0 + (time_sub_end.tv_nsec - time_sub_start.tv_nsec)/1000000000.0; \
    tprintf(PERF_COUNT_REPORT_STR, funct_name, SUB, elapsed_time_sec); \
    time_sub_start = time_sub_end;
#else
#define PERF_COUNT_SUB(SUB)
#endif

#endif
/**************************************************************************
 * enable/disable use of OpenCL
 **************************************************************************/

#ifdef USE_OPENCL

#define USE_DEVICE_SELECTION 1

#include "opencl_device_selection.h"

#ifndef strcasecmp
#define strcasecmp strcmp
#endif

#define MAX_KERNEL_STRING_LEN 64
#define MAX_CLFILE_NUM 50
#define MAX_CLKERNEL_NUM 200
#define MAX_KERNEL_NAME_LEN 64
#define CL_QUEUE_THREAD_HANDLE_AMD 0x403E
#define GROUPSIZE_X 16
#define GROUPSIZE_Y 16
#define GROUPSIZE_HMORX 256
#define GROUPSIZE_HMORY 1

typedef struct _KernelEnv
{
    cl_context mpkContext;
    cl_command_queue mpkCmdQueue;
    cl_program mpkProgram;
    cl_kernel mpkKernel;
    char mckKernelName[150];
} KernelEnv;

typedef struct _OpenCLEnv
{
    cl_platform_id mpOclPlatformID;
    cl_context mpOclContext;
    cl_device_id mpOclDevsID;
    cl_command_queue mpOclCmdQueue;
} OpenCLEnv;
typedef int ( *cl_kernel_function )( void **userdata, KernelEnv *kenv );


static l_int32 MORPH_BC = ASYMMETRIC_MORPH_BC;

static const l_uint32 lmask32[] = {0x0,
    0x80000000, 0xc0000000, 0xe0000000, 0xf0000000,
    0xf8000000, 0xfc000000, 0xfe000000, 0xff000000,
    0xff800000, 0xffc00000, 0xffe00000, 0xfff00000,
    0xfff80000, 0xfffc0000, 0xfffe0000, 0xffff0000,
    0xffff8000, 0xffffc000, 0xffffe000, 0xfffff000,
    0xfffff800, 0xfffffc00, 0xfffffe00, 0xffffff00,
    0xffffff80, 0xffffffc0, 0xffffffe0, 0xfffffff0,
    0xfffffff8, 0xfffffffc, 0xfffffffe, 0xffffffff};

static const l_uint32 rmask32[] = {0x0,
    0x00000001, 0x00000003, 0x00000007, 0x0000000f,
    0x0000001f, 0x0000003f, 0x0000007f, 0x000000ff,
    0x000001ff, 0x000003ff, 0x000007ff, 0x00000fff,
    0x00001fff, 0x00003fff, 0x00007fff, 0x0000ffff,
    0x0001ffff, 0x0003ffff, 0x0007ffff, 0x000fffff,
    0x001fffff, 0x003fffff, 0x007fffff, 0x00ffffff,
    0x01ffffff, 0x03ffffff, 0x07ffffff, 0x0fffffff,
    0x1fffffff, 0x3fffffff, 0x7fffffff, 0xffffffff};

#define CHECK_OPENCL(status,name)    \
if( status != CL_SUCCESS )    \
{    \
    printf ("OpenCL error code is %d at   when %s .\n", status, name);    \
}


typedef struct _GPUEnv
{
    //share vb in all modules in hb library
    cl_platform_id mpPlatformID;
    cl_device_type mDevType;
    cl_context mpContext;
    cl_device_id *mpArryDevsID;
    cl_device_id mpDevID;
    cl_command_queue mpCmdQueue;
    cl_kernel mpArryKernels[MAX_CLFILE_NUM];
    cl_program mpArryPrograms[MAX_CLFILE_NUM]; //one program object maps one kernel source file
    char mArryKnelSrcFile[MAX_CLFILE_NUM][256], //the max len of kernel file name is 256
         mArrykernelNames[MAX_CLKERNEL_NUM][MAX_KERNEL_STRING_LEN + 1];
         cl_kernel_function mpArryKnelFuncs[MAX_CLKERNEL_NUM];
    int mnKernelCount, mnFileCount, // only one kernel file
        mnIsUserCreated; // 1: created , 0:no create and needed to create by opencl wrapper
    int mnKhrFp64Flag;
    int mnAmdFp64Flag;

} GPUEnv;


class OpenclDevice
{

public:
    static GPUEnv gpuEnv;
    static int isInited;
    OpenclDevice();
    ~OpenclDevice();
    static int InitEnv(); // load dll, call InitOpenclRunEnv(0)
    static int InitOpenclRunEnv( int argc ); // RegistOpenclKernel, double flags, compile kernels
    static int InitOpenclRunEnv_DeviceSelection( int argc ); // RegistOpenclKernel, double flags, compile kernels
    static int InitOpenclRunEnv( GPUEnv *gpu ); // select device by env_CPU or selector
    static int RegistOpenclKernel();
    static int ReleaseOpenclRunEnv();
    static int ReleaseOpenclEnv( GPUEnv *gpuInfo );
    static int CompileKernelFile( GPUEnv *gpuInfo, const char *buildOption );
    static int CachedOfKernerPrg( const GPUEnv *gpuEnvCached, const char * clFileName );
    static int GeneratBinFromKernelSource( cl_program program, const char * clFileName );
    static int WriteBinaryToFile( const char* fileName, const char* birary, size_t numBytes );
    static int BinaryGenerated( const char * clFileName, FILE ** fhandle );
    //static int CompileKernelFile( const char *filename, GPUEnv *gpuInfo, const char *buildOption );
    static l_uint32* pixReadFromTiffKernel(l_uint32 *tiffdata,l_int32 w,l_int32 h,l_int32 wpl, l_uint32 *line);
    static Pix* pixReadTiffCl( const char *filename, l_int32 n );
    static PIX * pixReadStreamTiffCl ( FILE *fp, l_int32 n );
	static PIX * pixReadMemTiffCl(const l_uint8 *data, size_t size, l_int32  n);
    static PIX* pixReadFromTiffStreamCl(TIFF  *tif);
    static int composeRGBPixelCl(int *tiffdata,int *line,int h,int w);
    static l_int32 getTiffStreamResolutionCl(TIFF *tif,l_int32  *pxres,l_int32  *pyres);
    static TIFF* fopenTiffCl(FILE *fp,const char  *modestring);

/* OpenCL implementations of Morphological operations*/
    
    //Initialiation of OCL buffers used in Morph operations
    static int initMorphCLAllocations(l_int32  wpl, l_int32  h, PIX* pixs);
    static void releaseMorphCLBuffers();

    // OpenCL implementation of Morphology Dilate
    static PIX* pixDilateBrickCL(PIX  *pixd, PIX  *pixs, l_int32  hsize, l_int32  vsize, bool reqDataCopy);
    
    // OpenCL implementation of Morphology Erode
    static PIX* pixErodeBrickCL(PIX  *pixd, PIX  *pixs, l_int32  hsize, l_int32  vsize, bool reqDataCopy);
    
    // OpenCL implementation of Morphology Close
    static PIX* pixCloseBrickCL(PIX  *pixd, PIX  *pixs, l_int32  hsize, l_int32  vsize, bool reqDataCopy);

    // OpenCL implementation of Morphology Open
    static PIX* pixOpenBrickCL(PIX  *pixd, PIX  *pixs, l_int32  hsize, l_int32  vsize, bool reqDataCopy);
    
    // OpenCL implementation of Morphology Open
    static PIX* pixSubtractCL(PIX  *pixd, PIX  *pixs1, PIX  *pixs2, bool reqDataCopy);

    // OpenCL implementation of Morphology (Hollow = Closed - Open)
    static PIX* pixHollowCL(PIX  *pixd, PIX  *pixs, l_int32  close_hsize, l_int32  close_vsize, l_int32  open_hsize, l_int32  open_vsize, bool reqDataCopy);

    static void pixGetLinesCL(PIX  *pixd, PIX  *pixs, 
                                            PIX** pix_vline, PIX** pix_hline, 
                                            PIX** pixClosed, bool  getpixClosed,
                                            l_int32  close_hsize, l_int32  close_vsize, 
                                            l_int32  open_hsize, l_int32  open_vsize,
                                            l_int32  line_hsize, l_int32  line_vsize);

    //int InitOpenclAttr( OpenCLEnv * env );
    //int ReleaseKernel( KernelEnv * env );
    static int SetKernelEnv( KernelEnv *envInfo );
    //int CreateKernel( char * kernelname, KernelEnv * env );
    //int RunKernel( const char *kernelName, void **userdata );
    //int ConvertToString( const char *filename, char **source );
    //int CheckKernelName( KernelEnv *envInfo, const char *kernelName );
    //int RegisterKernelWrapper( const char *kernelName, cl_kernel_function function );
    //int RunKernelWrapper( cl_kernel_function function, const char * kernelName, void **usrdata );
    //int GetKernelEnvAndFunc( const char *kernelName, KernelEnv *env, cl_kernel_function *function );
    // static cl_device_id performDeviceSelection( );
    //static bool thresholdRectToPixMicroBench( TessScoreEvaluationInputData input, ds_device_type type);

    static int LoadOpencl();
#ifdef WIN32
    //static int OpenclInite();
    static void FreeOpenclDll();
#endif

    //int GetOpenclState();
    //void SetOpenclState( int state );
    inline static int AddKernelConfig( int kCount, const char *kName );

    /* for binarization */
    static void HistogramRectOCL(
        const unsigned char *imagedata,
        int bytes_per_pixel,
        int bytes_per_line,
        int left,
        int top,
        int width,
        int height,
        int kHistogramSize,
        int *histogramAllChannels);
    static void ThresholdRectToPixOCL(
        const unsigned char* imagedata,
        int bytes_per_pixel,
        int bytes_per_line,
        const int* thresholds,
        const int* hi_values,
        Pix** pix,
        int rect_height,
        int rect_width,
        int rect_top,
        int rect_left);
#if USE_DEVICE_SELECTION
    static ds_device getDeviceSelection();
    static ds_device selectedDevice;
    static bool deviceIsSelected;
#endif
    static bool selectedDeviceIsOpenCL();
    static bool selectedDeviceIsNativeCPU();

};


#endif
