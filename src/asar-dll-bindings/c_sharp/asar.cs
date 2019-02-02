using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Linq;

namespace AsarCLR
{
    /// <summary>
    /// Contains various functions to apply patches.
    /// </summary>
    public static unsafe class Asar
    {
        const int expectedapiversion = 303;

        [DllImport("asar", EntryPoint = "asar_init", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool asar_init();

        [DllImport("asar", EntryPoint = "asar_close", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool asar_close();

        [DllImport("asar", EntryPoint = "asar_version", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        private static extern int asar_version();

        [DllImport("asar", EntryPoint = "asar_apiversion", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        private static extern int asar_apiversion();

        [DllImport("asar", EntryPoint = "asar_reset", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool asar_reset();

        [DllImport("asar", EntryPoint = "asar_patch", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool asar_patch(string patchLocation, byte* romData, int bufLen, int* romLength);

        [DllImport("asar", EntryPoint = "asar_patch_ex", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool asar_patch_ex(ref RawPatchParams parameters);

        [DllImport("asar", EntryPoint = "asar_maxromsize", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        private static extern int asar_maxromsize();

        [DllImport("asar", EntryPoint = "asar_geterrors", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        private static extern RawAsarError* asar_geterrors(out int length);

        [DllImport("asar", EntryPoint = "asar_getwarnings", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        private static extern RawAsarError* asar_getwarnings(out int length);

        [DllImport("asar", EntryPoint = "asar_getprints", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        private static extern void** asar_getprints(out int length);

        [DllImport("asar", EntryPoint = "asar_getalllabels", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        private static extern RawAsarLabel* asar_getalllabels(out int length);

        [DllImport("asar", EntryPoint = "asar_getlabelval", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        private static extern int asar_getlabelval(string labelName);

        [DllImport("asar", EntryPoint = "asar_getdefine", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr asar_getdefine(string defineName);

        [DllImport("asar", EntryPoint = "asar_getalldefines", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        private static extern RawAsarDefine* asar_getalldefines(out int length);

        [DllImport("asar", EntryPoint = "asar_resolvedefines", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr asar_resolvedefines(string data, bool learnNew);

        [DllImport("asar", EntryPoint = "asar_math", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        private static extern double asar_math(string math, out IntPtr error);

        [DllImport("asar", EntryPoint = "asar_getwrittenblocks", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        private static extern RawAsarWrittenBlock* asar_getwrittenblocks(out int length);

        [DllImport("asar", EntryPoint = "asar_getmapper", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        private static extern MapperType asar_getmapper();

        [DllImport("asar", EntryPoint = "asar_getsymbolsfile", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr asar_getsymbolsfile(string format);

        /// <summary>
        /// Loads and initializes the DLL. You must call this before using any other Asar function.
        /// </summary>
        /// <returns>True if success</returns>
        public static bool init()
        {
            try
            {
                if (apiversion() < expectedapiversion || (apiversion() / 100) > (expectedapiversion / 100))
                {
                    return false;
                }

                if (!asar_init())
                {
                    return false;
                }

                return true;
            }
            catch
            {
                return false;
            }
        }

        /// <summary>
        /// Closes Asar DLL. Call this when you're done using Asar functions.
        /// </summary>
        public static void close()
        {
            asar_close();
        }

        /// <summary>
        /// Returns the version, in the format major*10000+minor*100+bugfix*1.
        /// This means that 1.2.34 would be returned as 10234.
        /// </summary>
        /// <returns>Asar version</returns>
        public static int version()
        {
            return asar_version();
        }

        /// <summary>
        /// Returns the API version, format major*100+minor. Minor is incremented on backwards compatible
        ///  changes; major is incremented on incompatible changes. Does not have any correlation with the
        ///  Asar version.
        /// It's not very useful directly, since Asar.init() verifies this automatically.
        /// </summary>
        /// <returns>Asar API version</returns>
        public static int apiversion()
        {
            return asar_apiversion();
        }

        /// <summary>
        /// Clears out all errors, warnings and printed statements, and clears the file cache.
        /// Not useful for much, since patch() already does this.
        /// </summary>
        /// <returns>True if success</returns>
        public static bool reset()
        {
            return asar_reset();
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        private struct RawPatchParams
        {
            public int structsize;
            public string patchloc;
            public byte* romdata;
            public int buflen;
            public int* romlen;
            public byte** includepaths;
            public int numincludepaths;
            [MarshalAs(UnmanagedType.I1)]
            public bool should_reset;
            public RawAsarDefine* additional_defines;
            public int additional_define_count;
            public string stdincludesfile;
            public string stddefinesfile;
        };

        /// <summary>
        /// Applies a patch.
        /// </summary>
        /// <param name="patchLocation">The patch location.</param>
        /// <param name="romData">The rom data. It must not be headered.</param>
        /// <param name="includePaths">lists additional include paths</param>
        /// <param name="shouldReset">specifies whether asar should clear out all defines, labels,
        /// etc from the last inserted file. Setting it to False will make Asar act like the
        //  currently patched file was directly appended to the previous one.</param>
        /// <param name="additionalDefines">specifies extra defines to give to the patch</param>
        /// <param name="stdIncludeFile">path to a file that specifes additional include paths</param>
        /// <param name="stdDefineFile">path to a file that specifes additional defines</param>
        /// <returns>True if no errors.</returns>
        public static bool patch(string patchLocation, ref byte[] romData, string[] includePaths = null,
            bool shouldReset = true, Dictionary<string, string> additionalDefines = null,
            string stdIncludeFile = null, string stdDefineFile = null)
        {
            if (includePaths == null)
            {
                includePaths = new string[0];
            }

            if (additionalDefines == null)
            {
                additionalDefines = new Dictionary<string, string>();
            }

            var includes = new byte*[includePaths.Length];
            var defines = new RawAsarDefine[additionalDefines.Count];

            try
            {
                for (int i = 0; i < includePaths.Length; i++)
                {
                    includes[i] = (byte*)Marshal.StringToCoTaskMemAnsi(includePaths[i]);
                }

                var keys = additionalDefines.Keys.ToArray();

                for (int i = 0; i < additionalDefines.Count; i++)
                {
                    var name = keys[i];
                    var value = additionalDefines[name];
                    defines[i].name = Marshal.StringToCoTaskMemAnsi(name);
                    defines[i].contents = Marshal.StringToCoTaskMemAnsi(value);
                }

                int newsize = maxromsize();
                int length = romData.Length;

                if (length < newsize)
                {
                    Array.Resize(ref romData, newsize);
                }

                bool success;

                fixed (byte* ptr = romData)
                fixed (byte** includepaths = includes)
                fixed (RawAsarDefine* additional_defines = defines)
                {
                    var param = new RawPatchParams
                    {
                        patchloc = patchLocation,
                        romdata = ptr,
                        buflen = newsize,
                        romlen = &length,

                        should_reset = shouldReset,
                        includepaths = includepaths,
                        numincludepaths = includes.Length,
                        additional_defines = additional_defines,
                        additional_define_count = defines.Length,
                        stddefinesfile = stdDefineFile,
                        stdincludesfile = stdIncludeFile
                    };
                    param.structsize = Marshal.SizeOf(param);

                    success = asar_patch_ex(ref param);
                }

                if (length < newsize)
                {
                    Array.Resize(ref romData, length);
                }

                return success;
            }
            finally
            {
                for (int i = 0; i < includes.Length; i++)
                {
                    Marshal.FreeCoTaskMem((IntPtr)includes[i]);
                }

                foreach (var define in defines)
                {
                    Marshal.FreeCoTaskMem(define.name);
                    Marshal.FreeCoTaskMem(define.contents);
                }
            }
        }

        /// <summary>
        /// Returns the maximum possible size of the output ROM from asar_patch().
        /// Giving this size to buflen guarantees you will not get any buffer too small errors;
        /// however, it is safe to give smaller buffers if you don't expect any ROMs larger
        /// than 4MB or something. It's not very useful directly, since Asar.patch() uses this automatically.
        /// </summary>
        /// <returns>Maximum output size of the ROM.</returns>
        public static int maxromsize()
        {
            return asar_maxromsize();
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct RawAsarError
        {
            public IntPtr fullerrdata;
            public IntPtr rawerrdata;
            public IntPtr block;
            public IntPtr filename;
            public int line;
            public IntPtr callerfilename;
            public int callerline;
            public int errid;
        };

        private static Asarerror[] cleanerrors(RawAsarError* ptr, int length)
        {
            Asarerror[] output = new Asarerror[length];

            // Copy unmanaged to managed memory to avoid potential errors in case the area
            // gets cleared by Asar.
            for (int i = 0; i < length; i++)
            {
                output[i].Fullerrdata = Marshal.PtrToStringAnsi(ptr[i].fullerrdata);
                output[i].Rawerrdata = Marshal.PtrToStringAnsi(ptr[i].rawerrdata);
                output[i].Block = Marshal.PtrToStringAnsi(ptr[i].block);
                output[i].Filename = Marshal.PtrToStringAnsi(ptr[i].filename);
                output[i].Line = ptr[i].line;
                output[i].Callerfilename = Marshal.PtrToStringAnsi(ptr[i].callerfilename);
                output[i].Callerline = ptr[i].callerline;
                output[i].ErrorId = ptr[i].errid;
            }

            return output;
        }

        /// <summary>
        /// Gets all Asar current errors. They're safe to keep for as long as you want.
        /// </summary>
        /// <returns>All Asar's errors.</returns>
        public static Asarerror[] geterrors()
        {
            int length = 0;
            RawAsarError* ptr = asar_geterrors(out length);
            return cleanerrors(ptr, length);
        }

        /// <summary>
        /// Gets all Asar current warning. They're safe to keep for as long as you want.
        /// </summary>
        /// <returns>All Asar's warnings.</returns>
        public static Asarerror[] getwarnings()
        {
            int length = 0;
            RawAsarError* ptr = asar_getwarnings(out length);
            return cleanerrors(ptr, length);
        }

        /// <summary>
        /// Gets all prints generated by the patch
        /// (Note: to see warnings/errors, check getwarnings() and geterrors()
        /// </summary>
        /// <returns>All prints</returns>
        public static string[] getprints()
        {
            int length;
            void** ptr = asar_getprints(out length);

            string[] output = new string[length];

            for (int i = 0; i < length; i++)
            {
                output[i] = Marshal.PtrToStringAnsi((IntPtr)ptr[i]);
            }

            return output;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct RawAsarLabel
        {
            public IntPtr name;
            public int location;
        }

        /// <summary>
        /// Gets all Asar current labels. They're safe to keep for as long as you want.
        /// </summary>
        /// <returns>All Asar's labels.</returns>
        public static Asarlabel[] getlabels()
        {
            int length = 0;
            RawAsarLabel* ptr = asar_getalllabels(out length);
            Asarlabel[] output = new Asarlabel[length];

            // Copy unmanaged to managed memory to avoid potential errors in case the area
            // gets cleared by Asar.
            for (int i = 0; i < length; i++)
            {
                output[i].Name = Marshal.PtrToStringAnsi(ptr[i].name);
                output[i].Location = ptr[i].location;
            }

            return output;
        }

        /// <summary>
        /// Gets a value of a specific label. Returns "-1" if label has not found.
        /// </summary>
        /// <param name="labelName">The label name.</param>
        /// <returns>The value of label. If not found, it returns -1 here.</returns>
        public static int getlabelval(string labelName)
        {
            return asar_getlabelval(labelName);
        }

        /// <summary>
        /// Gets contents of a define. If define doesn't exists, a null string will be generated.
        /// </summary>
        /// <param name="defineName">The define name.</param>
        /// <returns>The define content. If define has not found, this will be null.</returns>
        public static string getdefine(string defineName)
        {
            return Marshal.PtrToStringAnsi(asar_getdefine(defineName));
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct RawAsarDefine
        {
            public IntPtr name;
            public IntPtr contents;
        }
        /// <summary>
        /// Gets all Asar current defines. They're safe to keep for as long as you want.
        /// </summary>
        /// <returns>All Asar's defines.</returns>
        public static Asardefine[] getalldefines()
        {
            int length = 0;
            RawAsarDefine* ptr = asar_getalldefines(out length);
            Asardefine[] output = new Asardefine[length];

            // Copy unmanaged to managed memory to avoid potential errors in case the area
            // gets cleared by Asar.
            for (int i = 0; i < length; i++)
            {
                output[i].Name = Marshal.PtrToStringAnsi(ptr[i].name);
                output[i].Contents = Marshal.PtrToStringAnsi(ptr[i].contents);
            }

            return output;
        }

        /// <summary>
        /// </summary>
        /// <param name="data"></param>
        /// <param name="learnNew"></param>
        /// <returns></returns>
        public static string resolvedefines(string data, bool learnNew)
        {
            return Marshal.PtrToStringAnsi(asar_resolvedefines(data, learnNew));
        }

        /// <summary>
        /// Parse a string of math.
        /// </summary>
        /// <param name="math">The math string, i.e "1+1"</param>
        /// <param name="error">If occurs any error, it will showed here.</param>
        /// <returns>Product.</returns>
        public static double math(string math, out string error)
        {
            IntPtr err = IntPtr.Zero;
            double value = asar_math(math, out err);

            error = Marshal.PtrToStringAnsi(err);
            return value;
        }


        [StructLayout(LayoutKind.Sequential)]
        private struct RawAsarWrittenBlock
        {
            public int pcoffset;
            public int snesoffset;
            public int numbytes;
        };

        private static Asarwrittenblock[] CleanWrittenBlocks(RawAsarWrittenBlock* ptr, int length)
        {
            Asarwrittenblock[] output = new Asarwrittenblock[length];

            // Copy unmanaged to managed memory to avoid potential errors in case the area
            // gets cleared by Asar.
            for (int i = 0; i < length; i++)
            {
                output[i].Snesoffset = ptr[i].snesoffset;
                output[i].Numbytes = ptr[i].numbytes;
                output[i].Pcoffset = ptr[i].pcoffset;
            }

            return output;
        }

        /// <summary>
        /// Gets all Asar blocks written to the ROM. They're safe to keep for as long as you want.
        /// </summary>
        /// <returns>All Asar's blocks written to the ROM.</returns>
        public static Asarwrittenblock[] getwrittenblocks()
        {
            int length = 0;
            RawAsarWrittenBlock* ptr = asar_getwrittenblocks(out length);
            return CleanWrittenBlocks(ptr, length);
        }

        /// <summary>
        /// Gets mapper currently used by Asar.
        /// </summary>
        /// <returns>Returns mapper currently used by Asar.</returns>
        public static MapperType getmapper()
        {
            MapperType mapper = asar_getmapper();
            return mapper;
        }

        /// <summary>
        /// Generates the contents of a symbols file for in a specific format.
        /// </summary>
        /// <param name="format">The symbol file format to generate</param>
        /// <returns>Returns the textual contents of the symbols file.</returns>
        public static string getsymbolsfile(string format = "wla")
        {
            return Marshal.PtrToStringAnsi(asar_getsymbolsfile(format));
        }
    }

    /// <summary>
    /// Contains full information of a Asar error or warning.
    /// </summary>
    public struct Asarerror
    {
        public string Fullerrdata;
        public string Rawerrdata;
        public string Block;
        public string Filename;
        public int Line;
        public string Callerfilename;
        public int Callerline;
        public int ErrorId;
    }

    /// <summary>
    /// Contains a label from Asar.
    /// </summary>
    public struct Asarlabel
    {
        public string Name;
        public int Location;
    }

    /// <summary>
    /// Contains a Asar define.
    /// </summary>
    public struct Asardefine
    {
        public string Name;
        public string Contents;
    }

    /// <summary>
    /// Contains full information on a block written to the ROM.
    /// </summary>
    public struct Asarwrittenblock
    {
        public int Pcoffset;
        public int Snesoffset;
        public int Numbytes;
    }

    /// <summary>
    /// Defines the ROM mapper used.
    /// </summary>
    public enum MapperType
    {
        /// <summary>
        /// Invalid map.
        /// </summary>
        InvalidMapper,

        /// <summary>
        /// Standard LoROM.
        /// </summary>
        LoRom,

        /// <summary>
        /// Standard HiROM.
        /// </summary>
        HiRom,

        /// <summary>
        /// SA-1 ROM.
        /// </summary>
        Sa1Rom,

        /// <summary>
        /// SA-1 ROM with 8 MB mapped at once.
        /// </summary>
        BigSa1Rom,

        /// <summary>
        /// Super FX ROM.
        /// </summary>
        SfxRom,

        /// <summary>
        /// ExLoROM.
        /// </summary>
        ExLoRom,

        /// <summary>
        /// ExHiROM.
        /// </summary>
        ExHiRom,

        /// <summary>
        /// No specific ROM mapping.
        /// </summary>
        NoRom
    }
}
