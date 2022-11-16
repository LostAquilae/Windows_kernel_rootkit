using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace winpoolmsc
{
    internal class HitFile
    {
        private string path;
        private List<string> filterHits;

        public HitFile(string _path)
        {
            this.path = _path;
            filterHits = new List<string>();
        }

        public void addHit(string hit)
        {
            this.filterHits.Add(hit);
        }

        public string Path
        {
            get { return path; }
        }
    }
}


