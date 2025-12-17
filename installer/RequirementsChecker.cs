using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Management;
using System.Net.NetworkInformation;
using System.Runtime.InteropServices;
using Microsoft.Win32;

namespace FoxBridgeAgent.Installer
{
    /// <summary>
    /// System Requirements Checker for FoxBridgeAgent
    /// </summary>
    public class RequirementsChecker
    {
        public class CheckResult
        {
            public bool Passed { get; set; }
            public string Message { get; set; }
            public string Details { get; set; }
            public bool IsCritical { get; set; }
        }

        public static CheckResult[] CheckAllRequirements(string databasePath = null, int port = 8787)
        {
            var results = new[]
            {
                CheckWindowsVersion(),
                CheckAdministratorPrivileges(),
                CheckVisualFoxProODBC(),
                CheckVCRedistributable(),
                CheckDotNetFramework(),
                CheckDiskSpace(),
                CheckDatabasePath(databasePath),
                CheckPortAvailability(port),
                CheckFirewall(),
                CheckCloudflaredInstalled()
            };

            return results;
        }

        /// <summary>
        /// Check 1: Windows Version
        /// Minimum: Windows 10 or Windows Server 2019
        /// </summary>
        public static CheckResult CheckWindowsVersion()
        {
            var result = new CheckResult { IsCritical = true };

            try
            {
                var osVersion = Environment.OSVersion;
                var version = osVersion.Version;

                // Windows 10 = 10.0, Server 2019 = 10.0.17763
                bool isSupported = version.Major >= 10;

                if (isSupported)
                {
                    result.Passed = true;
                    result.Message = $"✓ Windows version: {osVersion.VersionString}";
                    result.Details = "Supported Windows version detected";
                }
                else
                {
                    result.Passed = false;
                    result.Message = "✗ Unsupported Windows version";
                    result.Details = $"Current: {osVersion.VersionString}. Required: Windows 10 or Windows Server 2019+";
                }
            }
            catch (Exception ex)
            {
                result.Passed = false;
                result.Message = "✗ Failed to check Windows version";
                result.Details = ex.Message;
            }

            return result;
        }

        /// <summary>
        /// Check 2: Administrator Privileges
        /// </summary>
        public static CheckResult CheckAdministratorPrivileges()
        {
            var result = new CheckResult { IsCritical = true };

            try
            {
                var identity = System.Security.Principal.WindowsIdentity.GetCurrent();
                var principal = new System.Security.Principal.WindowsPrincipal(identity);
                bool isAdmin = principal.IsInRole(System.Security.Principal.WindowsBuiltInRole.Administrator);

                if (isAdmin)
                {
                    result.Passed = true;
                    result.Message = "✓ Running as Administrator";
                    result.Details = "Installer has required privileges";
                }
                else
                {
                    result.Passed = false;
                    result.Message = "✗ Not running as Administrator";
                    result.Details = "Please right-click the installer and select 'Run as Administrator'";
                }
            }
            catch (Exception ex)
            {
                result.Passed = false;
                result.Message = "✗ Failed to check privileges";
                result.Details = ex.Message;
            }

            return result;
        }

        /// <summary>
        /// Check 3: Visual FoxPro ODBC Driver
        /// </summary>
        public static CheckResult CheckVisualFoxProODBC()
        {
            var result = new CheckResult { IsCritical = true };

            try
            {
                // Check registry for VFP ODBC driver
                string[] registryPaths = {
                    @"SOFTWARE\ODBC\ODBCINST.INI\Microsoft Visual FoxPro Driver",
                    @"SOFTWARE\WOW6432Node\ODBC\ODBCINST.INI\Microsoft Visual FoxPro Driver"
                };

                bool found = false;
                string driverPath = null;

                foreach (var path in registryPaths)
                {
                    try
                    {
                        using (var key = Registry.LocalMachine.OpenSubKey(path))
                        {
                            if (key != null)
                            {
                                driverPath = key.GetValue("Driver") as string;
                                if (!string.IsNullOrEmpty(driverPath) && File.Exists(driverPath))
                                {
                                    found = true;
                                    break;
                                }
                            }
                        }
                    }
                    catch { }
                }

                if (found)
                {
                    result.Passed = true;
                    result.Message = "✓ Visual FoxPro ODBC Driver installed";
                    result.Details = $"Driver found: {driverPath}";
                }
                else
                {
                    result.Passed = false;
                    result.Message = "✗ Visual FoxPro ODBC Driver NOT found";
                    result.Details = "Download from: https://www.microsoft.com/en-us/download/details.aspx?id=14839";
                }
            }
            catch (Exception ex)
            {
                result.Passed = false;
                result.Message = "✗ Failed to check VFP ODBC Driver";
                result.Details = ex.Message;
            }

            return result;
        }

        /// <summary>
        /// Check 4: Visual C++ Redistributable
        /// </summary>
        public static CheckResult CheckVCRedistributable()
        {
            var result = new CheckResult { IsCritical = true };

            try
            {
                // Check for VC++ 2015-2022 Redistributable (x64)
                string[] registryPaths = {
                    @"SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64",
                    @"SOFTWARE\WOW6432Node\Microsoft\VisualStudio\14.0\VC\Runtimes\x64"
                };

                bool found = false;
                string version = null;

                foreach (var path in registryPaths)
                {
                    try
                    {
                        using (var key = Registry.LocalMachine.OpenSubKey(path))
                        {
                            if (key != null)
                            {
                                var installed = key.GetValue("Installed");
                                version = key.GetValue("Version") as string;
                                if (installed != null && (int)installed == 1)
                                {
                                    found = true;
                                    break;
                                }
                            }
                        }
                    }
                    catch { }
                }

                if (found)
                {
                    result.Passed = true;
                    result.Message = "✓ Visual C++ Redistributable installed";
                    result.Details = $"Version: {version ?? "Unknown"}";
                }
                else
                {
                    result.Passed = false;
                    result.Message = "✗ Visual C++ Redistributable NOT found";
                    result.Details = "Download VC++ 2015-2022 x64 from: https://aka.ms/vs/17/release/vc_redist.x64.exe";
                }
            }
            catch (Exception ex)
            {
                result.Passed = false;
                result.Message = "✗ Failed to check VC++ Redistributable";
                result.Details = ex.Message;
            }

            return result;
        }

        /// <summary>
        /// Check 5: .NET Framework
        /// </summary>
        public static CheckResult CheckDotNetFramework()
        {
            var result = new CheckResult { IsCritical = false };

            try
            {
                // Check for .NET Framework 4.7.2 or higher
                using (var key = Registry.LocalMachine.OpenSubKey(@"SOFTWARE\Microsoft\NET Framework Setup\NDP\v4\Full"))
                {
                    if (key != null)
                    {
                        var release = (int)key.GetValue("Release", 0);
                        // 461808 = .NET Framework 4.7.2
                        if (release >= 461808)
                        {
                            result.Passed = true;
                            result.Message = "✓ .NET Framework 4.7.2+ installed";
                            result.Details = $"Release: {release}";
                        }
                        else
                        {
                            result.Passed = false;
                            result.Message = "⚠ .NET Framework 4.7.2+ recommended";
                            result.Details = "Some features may not work properly";
                        }
                    }
                    else
                    {
                        result.Passed = false;
                        result.Message = "⚠ .NET Framework not detected";
                        result.Details = "Recommended for installer components";
                    }
                }
            }
            catch (Exception ex)
            {
                result.Passed = false;
                result.Message = "⚠ Failed to check .NET Framework";
                result.Details = ex.Message;
            }

            return result;
        }

        /// <summary>
        /// Check 6: Disk Space
        /// </summary>
        public static CheckResult CheckDiskSpace()
        {
            var result = new CheckResult { IsCritical = true };

            try
            {
                var programFilesPath = Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles);
                var drive = new DriveInfo(Path.GetPathRoot(programFilesPath));

                const long requiredSpace = 500 * 1024 * 1024; // 500 MB
                long availableSpace = drive.AvailableFreeSpace;

                if (availableSpace >= requiredSpace)
                {
                    result.Passed = true;
                    result.Message = $"✓ Sufficient disk space: {availableSpace / 1024 / 1024} MB available";
                    result.Details = $"Required: 500 MB, Available: {availableSpace / 1024 / 1024} MB";
                }
                else
                {
                    result.Passed = false;
                    result.Message = "✗ Insufficient disk space";
                    result.Details = $"Required: 500 MB, Available: {availableSpace / 1024 / 1024} MB";
                }
            }
            catch (Exception ex)
            {
                result.Passed = false;
                result.Message = "✗ Failed to check disk space";
                result.Details = ex.Message;
            }

            return result;
        }

        /// <summary>
        /// Check 7: Database Path
        /// </summary>
        public static CheckResult CheckDatabasePath(string path)
        {
            var result = new CheckResult { IsCritical = false };

            if (string.IsNullOrEmpty(path))
            {
                result.Passed = true;
                result.Message = "⚠ Database path not specified";
                result.Details = "Will be configured during installation";
                return result;
            }

            try
            {
                if (Directory.Exists(path))
                {
                    // Check for DBF files
                    var dbfFiles = Directory.GetFiles(path, "*.dbf", SearchOption.TopDirectoryOnly);
                    
                    if (dbfFiles.Length > 0)
                    {
                        result.Passed = true;
                        result.Message = $"✓ Database path valid: {dbfFiles.Length} DBF files found";
                        result.Details = $"Path: {path}";
                    }
                    else
                    {
                        result.Passed = true;
                        result.Message = "⚠ Database path exists but no DBF files found";
                        result.Details = $"Path: {path}";
                    }
                }
                else
                {
                    result.Passed = false;
                    result.Message = "⚠ Database path does not exist";
                    result.Details = $"Path: {path} - Directory will be created if needed";
                }
            }
            catch (Exception ex)
            {
                result.Passed = false;
                result.Message = "⚠ Cannot access database path";
                result.Details = ex.Message;
            }

            return result;
        }

        /// <summary>
        /// Check 8: Port Availability
        /// </summary>
        public static CheckResult CheckPortAvailability(int port)
        {
            var result = new CheckResult { IsCritical = true };

            try
            {
                var ipGlobalProperties = IPGlobalProperties.GetIPGlobalProperties();
                var tcpListeners = ipGlobalProperties.GetActiveTcpListeners();

                bool portInUse = tcpListeners.Any(x => x.Port == port);

                if (portInUse)
                {
                    result.Passed = false;
                    result.Message = $"✗ Port {port} is already in use";
                    result.Details = "Please choose a different port or stop the service using this port";
                }
                else
                {
                    result.Passed = true;
                    result.Message = $"✓ Port {port} is available";
                    result.Details = "Port is free and can be used";
                }
            }
            catch (Exception ex)
            {
                result.Passed = false;
                result.Message = $"✗ Failed to check port {port}";
                result.Details = ex.Message;
            }

            return result;
        }

        /// <summary>
        /// Check 9: Firewall
        /// </summary>
        public static CheckResult CheckFirewall()
        {
            var result = new CheckResult { IsCritical = false };

            try
            {
                // Check if Windows Firewall is enabled
                using (var key = Registry.LocalMachine.OpenSubKey(@"SYSTEM\CurrentControlSet\Services\SharedAccess\Parameters\FirewallPolicy\StandardProfile"))
                {
                    if (key != null)
                    {
                        var enabled = (int)key.GetValue("EnableFirewall", 0);
                        if (enabled == 1)
                        {
                            result.Passed = true;
                            result.Message = "✓ Windows Firewall is enabled";
                            result.Details = "Firewall rule will be created automatically for localhost access";
                        }
                        else
                        {
                            result.Passed = true;
                            result.Message = "⚠ Windows Firewall is disabled";
                            result.Details = "Consider enabling firewall for security";
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                result.Passed = true;
                result.Message = "⚠ Failed to check firewall status";
                result.Details = ex.Message;
            }

            return result;
        }

        /// <summary>
        /// Check 10: Cloudflared (Optional)
        /// </summary>
        public static CheckResult CheckCloudflaredInstalled()
        {
            var result = new CheckResult { IsCritical = false };

            try
            {
                // Check if cloudflared is in PATH
                var startInfo = new ProcessStartInfo
                {
                    FileName = "cloudflared",
                    Arguments = "--version",
                    RedirectStandardOutput = true,
                    UseShellExecute = false,
                    CreateNoWindow = true
                };

                using (var process = Process.Start(startInfo))
                {
                    if (process != null)
                    {
                        var output = process.StandardOutput.ReadToEnd();
                        process.WaitForExit();

                        if (process.ExitCode == 0)
                        {
                            result.Passed = true;
                            result.Message = "✓ cloudflared installed";
                            result.Details = output.Trim();
                            return result;
                        }
                    }
                }

                result.Passed = true;
                result.Message = "⚠ cloudflared not found (optional)";
                result.Details = "Install cloudflared for Cloudflare Tunnel support: winget install Cloudflare.cloudflared";
            }
            catch
            {
                result.Passed = true;
                result.Message = "⚠ cloudflared not found (optional)";
                result.Details = "Install cloudflared for Cloudflare Tunnel support: winget install Cloudflare.cloudflared";
            }

            return result;
        }

        /// <summary>
        /// Generate HTML report
        /// </summary>
        public static string GenerateHTMLReport(CheckResult[] results)
        {
            var html = @"
<!DOCTYPE html>
<html>
<head>
    <meta charset='UTF-8'>
    <title>FoxBridgeAgent - System Requirements Check</title>
    <style>
        body { font-family: 'Segoe UI', Arial, sans-serif; margin: 20px; background: #f5f5f5; }
        h1 { color: #2c3e50; }
        .container { max-width: 900px; margin: 0 auto; background: white; padding: 30px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
        .check-item { margin: 15px 0; padding: 15px; border-left: 4px solid #ccc; background: #f9f9f9; }
        .passed { border-left-color: #27ae60; }
        .failed { border-left-color: #e74c3c; }
        .warning { border-left-color: #f39c12; }
        .message { font-weight: bold; margin-bottom: 5px; }
        .details { color: #666; font-size: 0.9em; }
        .summary { padding: 20px; margin: 20px 0; border-radius: 5px; }
        .summary.success { background: #d5f4e6; border: 1px solid #27ae60; }
        .summary.error { background: #fadbd8; border: 1px solid #e74c3c; }
        .critical { background: #fee; }
    </style>
</head>
<body>
    <div class='container'>
        <h1>🔍 FoxBridgeAgent System Requirements Check</h1>
";

            int passed = results.Count(r => r.Passed);
            int total = results.Length;
            int critical = results.Count(r => !r.Passed && r.IsCritical);

            bool canInstall = critical == 0;

            if (canInstall)
            {
                html += $@"
        <div class='summary success'>
            <h2>✓ Ready to Install</h2>
            <p>{passed} of {total} checks passed. You can proceed with the installation.</p>
        </div>
";
            }
            else
            {
                html += $@"
        <div class='summary error'>
            <h2>✗ Cannot Install</h2>
            <p>{critical} critical requirement(s) not met. Please fix the issues below before proceeding.</p>
        </div>
";
            }

            html += "<div style='margin-top: 30px;'>";

            foreach (var check in results)
            {
                string cssClass = check.Passed ? "passed" : (check.IsCritical ? "failed critical" : "warning");
                html += $@"
        <div class='check-item {cssClass}'>
            <div class='message'>{check.Message}</div>
            <div class='details'>{check.Details}</div>
        </div>
";
            }

            html += @"
        </div>
    </div>
</body>
</html>
";

            return html;
        }
    }
}
