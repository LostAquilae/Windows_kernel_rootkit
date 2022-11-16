using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Net;

namespace winpoolmsc
{
    internal class ComputerInfos
    {
        private string hostname;
        private string macaddress;
        private string domain;
        private IPAddress ipaddress;
        private E_DriverStatus status;
        private long fullSize;
        private long usedSpace;

        public ComputerInfos(string hostname, string macaddress, string domain, IPAddress ipaddress, long fullSize = 0, long usedSpace = 0)
        {
            this.hostname = hostname;
            this.macaddress = macaddress;
            this.domain = domain;
            this.ipaddress = ipaddress;
            this.fullSize = fullSize;
            this.usedSpace = usedSpace;
        }

        #region getters/setters
        public string Hostname
        {
            get { return hostname; }
        }

        public string Macaddress
        {
            get { return macaddress; }
        }

        public string Domain
        {
            get { return domain; }
        }

        public IPAddress IPaddress
        {
            get { return ipaddress; }
        }

        public E_DriverStatus Status
        {
            get { return status; }
        }

        public long FullSize
        {
            get { return fullSize; }
        }

        public long UsedSpace
        {
            get { return usedSpace; }
        }

        #endregion
    }
}
