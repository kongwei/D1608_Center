//---------------------------------------------------------------------------


#pragma hdrstop

#include "untNetwork.h"
#include <Dialogs.hpp>
#include <iostream>
#include <string.h>
#include <Iphlpapi.h>
//---------------------------------------------------------------------------

#pragma package(smart_init)
#pragma comment(lib,"Iphlpapi.lib")

String ParseData(PIP_ADAPTER_INFO pinfo);

String GetMacList()
{
    PIP_ADAPTER_INFO pinfo;
    unsigned long len = 0;
    unsigned  long nError;
    String result;
    nError = GetAdaptersInfo(pinfo, &len);
    if (nError==0)
    {
        result = ParseData(pinfo);
    }
    if (nError==ERROR_NO_DATA)
    {
        ShowMessage("没有网络设备信息");
    }
    if (nError==ERROR_NOT_SUPPORTED)
    {
        ShowMessage("GetAdaptersInfo不支持本系统");
    }
    if (nError==ERROR_BUFFER_OVERFLOW)
    {
        pinfo = (PIP_ADAPTER_INFO)malloc(len);
        nError = GetAdaptersInfo(pinfo,&len);
        if (nError==0)
        {
            result = ParseData(pinfo);            
        }
    }

    if (pinfo!=NULL)
        free(pinfo);

    return result;
}
String ParseData(PIP_ADAPTER_INFO pinfo)
{
    //String description, macaddress, type, IpAddress, subnet;
    String result;
    while (pinfo!=NULL)
    {
        String macaddress;
        macaddress.sprintf("%02X-%02X-%02X-%02X-%02X-%02X",pinfo->Address[0],pinfo->Address[1],pinfo->Address[2],pinfo->Address[3],pinfo->Address[4],pinfo->Address[5]);
        result = result + macaddress + ",";
        //description = pinfo->Description;
        //type.sprintf("%d",pinfo->Type);

        //PIP_ADDR_STRING pAddressList = &(pinfo->IpAddressList);
        //IpAddress = "";
        /*do
        {
            IpAddress += pAddressList->IpAddress.String;
            pAddressList = pAddressList->Next;
            if (pAddressList != NULL)
                IpAddress +="\r\n";
        } while (pAddressList != NULL);*/

        /*subnet.Format("%s",pinfo->IpAddressList.IpMask.String);
        gateway.Format("%s",pinfo->GatewayList.IpAddress.String);
        if (pinfo->HaveWins) 
            PrimaryWinsServer.Format("%s",pinfo->PrimaryWinsServer.IpAddress.String );
        else
            PrimaryWinsServer.Format("%s","N/A" );
        if (pinfo->DhcpEnabled )
            dhcp.Format("%s",pinfo->DhcpServer.IpAddress.String );
        else
            dhcp.Format("%s","N/A");*/
        pinfo = pinfo->Next;
    }
    //ShowMessage("网络设备为:\t"+description);
    //ShowMessage("Mac地址为:\t"+macaddress);
    //ShowMessage("网卡类型:\t"+type);
    //ShowMessage("IP地址:\t"+IpAddress);
    //ShowMessage("子网掩码:\t"+subnet);
    //ShowMessage("网关:\t"+gateway);
    //ShowMessage("主Wins服务器:\t"+PrimaryWinsServer);
    //ShowMessage("dhcp服务器:\t"+dhcp);

    return result;
}

