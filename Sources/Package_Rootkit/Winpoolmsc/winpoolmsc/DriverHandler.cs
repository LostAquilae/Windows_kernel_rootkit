using System.Management.Automation;
using System.ServiceProcess;

namespace winpoolmsc
{
    static internal class DriverHandler
    {
        private const string DRIVER_NAME = "tiboutik";
        

        public static E_DriverStatus GetDriverStatus()
        {
            ServiceController sc = new ServiceController();
            ServiceController[] scs = ServiceController.GetDevices("localhost");
            foreach (ServiceController service in scs)
            {
                if (service.ServiceName == DRIVER_NAME)
                {
                    switch (service.Status)
                    {
                        case ServiceControllerStatus.Running:
                            return E_DriverStatus.DRIVER_RUNNING;

                        case ServiceControllerStatus.Stopped:
                            return E_DriverStatus.DRIVER_STOPPED;

                        case ServiceControllerStatus.Paused:
                            return E_DriverStatus.DRIVER_PAUSED;

                        default:
                            return E_DriverStatus.DRIVER_UNKNOWN;
                    }
                }
            }

            return E_DriverStatus.DRIVER_UNKNOWN;

        }

    }
}
