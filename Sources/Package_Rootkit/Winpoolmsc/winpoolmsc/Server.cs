using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace winpoolmsc
{
    internal class Server
    {
        private string? hostname;
        private string address;
        private int port;
        private int lastPing;

        public Server(string? _hostname, string _address, int _port)
        {
            this.hostname = _hostname;
            this.address = _address;
            this.port = _port;
        }

        public string? Hostname
        {
            get { return this.hostname; }
        }
        
        public string? Address
        {
            get { return this.address; }
        }

        public int LastPing
        {
            get { return this.lastPing; }
        }

        public int Port
        {
            get { return this.port; }
        }

    }
}
