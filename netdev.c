#define _DEFAULT_SOURCE

/* main imports */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* linux kernel networking subsystem libraries */
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_link.h>

/* for socket communication */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

void handle_interface_flags(int sock_fd, struct ifreq interface, unsigned short flags);

/* helper function prototypes to show network interface information */
char *return_interface_netmask(int sock_fd, struct ifreq interface);
char *return_broadcast_addr(int sock_fd, struct ifreq interface);
char *return_hardware_addr(int sock_fd, struct ifreq interface);

/* network interface flags */
#define IFR_FLAGS interface.ifr_ifru.ifru_flags


int main(void)
{
    /*
        In order to communicate with the LKNS network interfaces
        we must create a socket file descriptor to establish a communication channel
    */
    int sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

    if (sock_fd < 0)
    {
        fprintf(stderr, "failed to create a suitable socket!\n");

        return -1;
    }

    struct ifreq interface; // generic network interface structure
    struct ifconf ifconfig; // interface configuration

    char buffer[1024]; // data storage opts

    /* obtain rough length of total interfaces available */
    ifconfig.ifc_len = sizeof(buffer);
    ifconfig.ifc_ifcu.ifcu_buf = buffer;

    /*
        Get a list of network interfaces
    */
    if (ioctl(sock_fd, SIOCGIFCONF, &ifconfig) < 0)
    {
        fprintf(stderr, "Failed to get any network interfaces on host!\n");

        return -1;
    }

    struct ifreq *iface = ifconfig.ifc_ifcu.ifcu_req;
    const struct ifreq *end = iface + (ifconfig.ifc_len / sizeof(struct ifreq));

    for (; iface != end; iface++)
    {
        char name[IFNAMSIZ];

        size_t name_sz = snprintf(NULL, 0, "%s", iface->ifr_ifrn.ifrn_name);

        if (name_sz < 1)
        {
            fprintf(stderr, "No device: ??? (%ld)\n", name_sz);
        }
        
        /* copy interface name into char buffer */
        strncpy(name, iface->ifr_ifrn.ifrn_name, name_sz);
        name[name_sz] = '\0'; // properly NUL terminate string

        /* show interface name */
        printf("%s:", name);

        /* wipe interface struct, set field members to 0 */
        memset(&interface, 0, sizeof(struct ifreq));
        strncpy(interface.ifr_ifrn.ifrn_name, name, IFNAMSIZ - 1);

        /* retrieve IP address */
        if (ioctl(sock_fd, SIOCGIFADDR, &interface) == 0)
        {
            /* retrieve address in presentation format */
            char addr_str[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET, &((struct sockaddr_in *)&interface.ifr_ifru.ifru_addr)->sin_addr, addr_str, sizeof(addr_str));

            size_t addr_sz = snprintf(NULL, 0, "%s", addr_str);
            addr_str[addr_sz] = '\0';

            printf("\n\tinet %s\n", addr_str);
        }

        char *netmask = return_interface_netmask(sock_fd, interface);

        if (netmask == NULL)
        {
            fprintf(stderr, "Failed to allocate space for netmask address!\n");

            return -1;
        }

        printf("\tnetmask %s\n", netmask);

        /* retrieve the ethernet address of the network interface card */
        if (ioctl(sock_fd, SIOCGIFHWADDR, &interface) == 0)
        {
            unsigned char *ethernet = (unsigned char *)&interface.ifr_ifru.ifru_hwaddr.sa_data;

            printf("\tether %02x:%02x:%02x:%02x:%02x:%02x\n",
                ethernet[0], ethernet[1],
                ethernet[2], ethernet[3],
                ethernet[4], ethernet[5]
            );
        }

        /* get network interface flags */
        if (ioctl(sock_fd, SIOCGIFFLAGS, &interface) == 0)
        {
            handle_interface_flags(sock_fd, interface, IFR_FLAGS);
        }
        else 
        {
            perror("???: ");
        }

        printf("\n");
    }

    close(sock_fd);

    return 0;
}


/* Display interface netmask address */
char *return_interface_netmask(int sock_fd, struct ifreq interface)
{
    /* retrieve netmask */
    if (ioctl(sock_fd, SIOCGIFNETMASK, &interface) == 0)
    {
        char netmask[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET, &((struct sockaddr_in *)&interface.ifr_ifru.ifru_netmask)->sin_addr, netmask, sizeof(netmask));
    
        size_t netmask_sz = snprintf(NULL, 0, "%s", netmask);
        netmask[netmask_sz] = '\0';

        char *ret_netmask = (char *) malloc(netmask_sz * sizeof(ret_netmask));

        if (ret_netmask == NULL)
        {
            return NULL;
        }

        sprintf(ret_netmask, "%s", netmask);
        return ret_netmask;
    }

    return (char *)"0.0.0.0";
}


/* Display interface broadcast address */
char *return_broadcast_addr(int sock_fd, struct ifreq interface)
{
    if (ioctl(sock_fd, SIOCGIFBRDADDR, &interface) == 0)
    {
        char broadcast[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET, &((struct sockaddr_in *)&interface.ifr_ifru.ifru_broadaddr)->sin_addr, broadcast, sizeof(broadcast));

        size_t broadcast_sz = snprintf(NULL, 0, "%s", broadcast);
        broadcast[broadcast_sz] = '\0';

        char *ret_broadcast = (char *) malloc(broadcast_sz * sizeof(ret_broadcast));

        if (ret_broadcast == NULL)
        {
            return NULL;
        }

        sprintf(ret_broadcast, "%s", broadcast);
        return ret_broadcast;
    }

    return (char *)"0.0.0.0";
}


/* Handle biface flags */
void handle_interface_flags(int sock_fd, struct ifreq interface, unsigned short flags)
{
    /* Interface is running */
    if (flags & IFF_UP)
    {
        printf("\tStatus: \033[0;32mACTIVE\033[0;m\n");
    }

    /* Valid broadcast address set */
    if (flags & IFF_BROADCAST)
    {
        char *broadcast_addr = return_broadcast_addr(sock_fd, interface);

        if (broadcast_addr == NULL)
        {
            fprintf(stderr, "Failed to allocate memory for broadcast address!\n");

            return;
        }

        printf("\tbroadcast %s\n", broadcast_addr);
        free(broadcast_addr);
    }

    /* Internal debugging flag */
    if (flags & IFF_DEBUG)
    {
        printf("\tDEBUG\n");
    }

    /* Interface is a loopback interface */
    if (flags & IFF_LOOPBACK)
    {
        printf("\tLOOPBACK\n");
    }

    /* Interface is a point-to-point link */
    if (flags & IFF_POINTOPOINT)
    {
        printf("\tPTP\n");
    }

    /* Resources allocated */
    if (flags & IFF_RUNNING)
    {
        printf("\tALLOCATED\n");
    }

    /* No arp protocol, L2 destination address not set */
    if (flags & IFF_NOARP)
    {
        printf("\tNOARP\n");
    }

    /* Interface is in promiscuous mode */
    if (flags & IFF_PROMISC)
    {
        printf("\tPROMISC\n");
    }

    /* Avoid use of trailers */
    if (flags & IFF_NOTRAILERS)
    {
        printf("\t!TRAILER\n");
    }

    /* Receive all multicast packets */
    if (flags & IFF_ALLMULTI)
    {
        printf("\tALLMUTLI\n");
    }

    /* Master of a load balancing bundle */
    if (flags & IFF_MASTER)
    {
        printf("\tMASTER-LBB\n");
    }

    /* Slave of a load balancing bundle */
    if (flags & IFF_SLAVE)
    {
        printf("\tSLAVE-LBB");
    }

    /* Supports multicast */
    if (flags & IFF_MULTICAST)
    {
        printf("\tMCAST-SUPPORT\n");
    }

    /* Is able to select media type via ifmap */
    if (flags & IFF_PORTSEL)
    {
        printf("\tMTYPE-IFMAP\n");
    }

    /* Auto media selection active */
    if (flags & IFF_AUTOMEDIA)
    {
        printf("\tAUTOM-ACTIVE\n");
    }

    /* The addresses are lost when the interface goes down */
    if (flags & IFF_DYNAMIC)
    {
        printf("\tLOST-DOWN\n");
    }

    return;
}
