using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;

namespace winpoolmsc
{
    static internal class FileHandler
    {

        public static List<HitFile> CrawlFiles(string filter, string entrypoint)
        {
            Console.WriteLine("Crawling {0}", entrypoint);
            List<HitFile> hitfiles = new List<HitFile>();
            foreach (string f in Directory.GetDirectories(entrypoint))
            {
                List <HitFile> recFiles = new List<HitFile>();
                try
                {
                    recFiles = CrawlFiles(filter, f);
                } catch(Exception ex) {
                    Console.WriteLine("Errors while crawling {0}", entrypoint);
                };
                
                foreach (HitFile recFile in recFiles)
                {
                    hitfiles.Add(recFile);
                }
            }

            foreach (string f in Directory.EnumerateFiles(entrypoint))
            {
                if (f.Contains(filter))
                {
                    HitFile file = new HitFile(f);
                    file.addHit(filter);
                    hitfiles.Add(file);
                }
            } 

            return hitfiles;
        }
    }
}
