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
        const int expectedapiversion = 400;

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
        private static extern bool asar_patch(ref RawPatchParams parameters);

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
        private static extern IntPtr asar_resolvedefines(string data);

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

        [StructLayout(LayoutKind.Sequential)]
        private struct RawWarnSetting
        {
            public byte* warnid;
            [MarshalAs(UnmanagedType.I1)]
            public bool enabled;
        };

        [StructLayout(LayoutKind.Sequential)]
        private struct RawMemoryFile
        {
            public byte* path;
            public void* buffer;
            public UIntPtr length;
        };

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
            public RawAsarDefine* additional_defines;
            public int additional_define_count;
            public string stdincludesfile;
            public string stddefinesfile;
            public RawWarnSetting* warning_settings;
            public int warning_setting_count;
            public RawMemoryFile* memory_files;
            public int memory_file_count;
            [MarshalAs(UnmanagedType.I1)]
            public bool override_checksum_gen;
            [MarshalAs(UnmanagedType.I1)]
            public bool generate_checksum;
            [MarshalAs(UnmanagedType.I1)]
            public bool full_call_stack;
        };

        /// <summary>
        /// Applies a patch.
        /// </summary>
        /// <param name="patchLocation">The patch location.</param>
        /// <param name="romData">The rom data. It must not be headered.</param>
        /// <param name="includePaths">lists additional include paths</param>
        /// <param name="additionalDefines">specifies extra defines to give to the patch</param>
        /// <param name="stdIncludeFile">path to a file that specifes additional include paths</param>
        /// <param name="stdDefineFile">path to a file that specifes additional defines</param>
        /// <param name="warningSettings">specifies enable/disable settings for each warning ID</param>
        /// <param name="memoryFiles">specifies a mapping for virtual file paths to file data stored
        /// in memory.</param>
        /// <param name="generateChecksum">specifies whether asar should generate a checksum. If this
        /// is null, the default behavior is used.</param>
        /// <param name="fullCallStack">whether warning and error messages should always
        /// contain the full call stack.</param>
        /// <returns>True if no errors.</returns>
        public static bool patch(string patchLocation, ref byte[] romData, string[] includePaths = null,
            Dictionary<string, string> additionalDefines = null,
            string stdIncludeFile = null, string stdDefineFile = null,
            Dictionary<string, bool> warningSettings = null, Dictionary<string, byte[]> memoryFiles = null,
            bool? generateChecksum = null, bool fullCallStack = false)
        {
            if (includePaths == null)
            {
                includePaths = new string[0];
            }

            if (additionalDefines == null)
            {
                additionalDefines = new Dictionary<string, string>();
            }

            if (warningSettings == null)
            {
                warningSettings = new Dictionary<string, bool>();
            }

            if (memoryFiles == null)
            {
                memoryFiles = new Dictionary<string, byte[]>();
            }

            var includes = new byte*[includePaths.Length];
            var defines = new RawAsarDefine[additionalDefines.Count];
            var warnings = new RawWarnSetting[warningSettings.Count];
            var memFiles = new RawMemoryFile[memoryFiles.Count];

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

                var warningKeys = warningSettings.Keys.ToArray();

                for (int i = 0; i < warningSettings.Count; i++)
                {
                    var warnId = warningKeys[i];
                    var value = warningSettings[warnId];
                    warnings[i].warnid = (byte*)Marshal.StringToCoTaskMemAnsi(warnId);
                    warnings[i].enabled = value;
                }

                var memFileKeys = memoryFiles.Keys.ToArray();

                for (int i = 0; i < memoryFiles.Count; i++)
                {
                    var path = memFileKeys[i];
                    var value = memoryFiles[path];
                    memFiles[i].path = (byte*)Marshal.StringToCoTaskMemAnsi(path);
                    fixed (byte* buf = &value[0])
                    {
                        memFiles[i].buffer = buf;
                    }
                    memFiles[i].length = (UIntPtr)value.Length;
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
                fixed (RawWarnSetting* warning_settings = warnings)
                fixed (RawMemoryFile* memory_files = memFiles)
                {
                    var param = new RawPatchParams
                    {
                        patchloc = patchLocation,
                        romdata = ptr,
                        buflen = newsize,
                        romlen = &length,

                        includepaths = includepaths,
                        numincludepaths = includes.Length,
                        additional_defines = additional_defines,
                        additional_define_count = defines.Length,
                        stddefinesfile = stdDefineFile,
                        stdincludesfile = stdIncludeFile,

                        warning_settings = warning_settings,
                        warning_setting_count = warnings.Length,
                        memory_files = memory_files,
                        memory_file_count = memFiles.Length,
                        override_checksum_gen = generateChecksum != null,
                        generate_checksum = generateChecksum ?? false,
                        full_call_stack = fullCallStack,
                    };
                    param.structsize = Marshal.SizeOf(param);

                    success = asar_patch(ref param);
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

                foreach (var warning in warnings)
                {
                    Marshal.FreeCoTaskMem((IntPtr)warning.warnid);
                }

                foreach (var memFile in memFiles)
                {
                    Marshal.FreeCoTaskMem((IntPtr)memFile.path);
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
            public IntPtr callstack;
            public int callstacksize;
            public IntPtr errname;
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
                //output[i].Callerfilename = Marshal.PtrToStringAnsi(ptr[i].callerfilename);
                //output[i].Callerline = ptr[i].callerline;
                output[i].ErrorId = Marshal.PtrToStringAnsi(ptr[i].errname);
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
        /// <returns></returns>
        public static string resolvedefines(string data)
        {
            return Marshal.PtrToStringAnsi(asar_resolvedefines(data));
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
        // TODO: call stack
        public string ErrorId;
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
