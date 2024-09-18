#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <arpa/inet.h>
#include <pcap/pcap.h>


void engage_network_interface(pcap_if_t *interface);

/* Network interface card helper function prototypes */
void return_interface_address(sa_family_t sa_family_short, pcap_addr_t *interface);
void return_interface_netmask(pcap_addr_t *interface);

#define CONFIG_SUCCESS "CONFIGURED"
#define CONFIG_ERROR "!CONFIGURED"

#define COLOR_GREY "\033[90;1m"
#define COLOR_GREEN "\033[0;32m"
#define COLOR_RED "\033[0;31m"

#define UNDERLINE "\033[0;4m"
#define OCTAL_ESC "\033[0;m"

#define ADDRESS_FORMAT_STRING printf("\t%s Address => \033[92;1m%s\033[0;m\n", internet_protocol_v6 > 0 ? "IPv6" : "IPv4", addr);
#define NETMASK_FORMAT_STRING printf("\t\t# NETMASK >> %s\n", netmask);

/* Interface address type */
int IFACE_ADDR_TYPE = 0;

/* Conversion helper (binary to presentation format) */
#define ADDRTOSTR inet_ntop(interface->addr->sa_family, &((struct sockaddr_in *)interface->addr)->sin_addr, addr, sizeof(addr));
#define BINTOFMT inet_ntop(interface->netmask->sa_family, &((struct sockaddr_in *)interface->netmask)->sin_addr, netmask, sizeof(netmask));


int main(int argc, char *argv[])
{
	if (argc > 1)
	{
		return -1;
	}

    const char *version = pcap_lib_version();
    printf("LSDEV - %s\n", version);

    size_t version_sz = snprintf(NULL, 0, "LSDEV - %s", version);

    for (size_t i = 0; i < version_sz; i++) 
	{
        printf("=");
    }

    printf("\n\n");

    int ctr = 0;

    pcap_if_t *devs_available, *singular_nic;

    char err_buff[PCAP_ERRBUF_SIZE];

    if (pcap_findalldevs(&devs_available, err_buff) == -1) 
	{
        fprintf(stderr, "./%s => failed to find a device: %s\n", argv[0], err_buff);
    
	    return -1;
    }

    // Obtain a struct of the total amount of network interface cards on the host
    for (singular_nic = devs_available; singular_nic != NULL; singular_nic = singular_nic->next) 
	{
        printf("#%s%d%s.) Name: %s%s%s | Status: ", UNDERLINE, ctr += 1, OCTAL_ESC, COLOR_GREY, singular_nic->name, OCTAL_ESC);
    
	    if (singular_nic->addresses != NULL) 
		{
            printf("%s%s%s\n", COLOR_GREEN, CONFIG_SUCCESS, OCTAL_ESC);
		} 
		else
		{
            printf("%s%s%s\n", COLOR_RED, CONFIG_ERROR, OCTAL_ESC);
        }

        engage_network_interface(singular_nic);
    }

    pcap_freealldevs(devs_available);

    return 0;
}


void engage_network_interface(pcap_if_t *interface)
{
	size_t i, x, iface_sz;
    pcap_addr_t *nic_address;

	i = x = iface_sz = 0;

    for (nic_address = interface->addresses; ; nic_address = nic_address->next) 
	{
		if (nic_address == NULL)
		{
			if (x > 0)
			{
				printf("\n");
			}
			else 
			{
				return;
			}

			return;
		}

        return_interface_address(nic_address->addr->sa_family, nic_address);

		x += 1;
    }

	return;
}


/*
	This function returns the associated netmask of the current interface field

	@param interface - pointer to a network interface card
*/
void return_interface_netmask(pcap_addr_t *interface)
{
	if (interface->netmask)
	{
		switch (IFACE_ADDR_TYPE)
		{
			case 0:
			{
				// Internet protocol version 4
				char netmask[INET_ADDRSTRLEN];

				BINTOFMT
				NETMASK_FORMAT_STRING
			}

			case 1:
			{
				// Internet protocol version 6
				char netmask[INET6_ADDRSTRLEN];

				BINTOFMT
				NETMASK_FORMAT_STRING
			}

			/* Should not be possible to route. Exit */
			default:
			{
				return;
			}
		}
	}

	return;
}


/*
	This function parses through the received structure and prints out each IPv4/IPv6 address
	accordingly.

	@param sa_family_short - a 2 byte integral value
	@param interface - pointer to a network interface structure
*/
void return_interface_address(sa_family_t sa_family_short, pcap_addr_t *interface)
{
	// Is this an IPv6 address? 0 (default) = no, otherwise true.
	int internet_protocol_v6 = 0;

	switch (sa_family_short) 
	{
		case AF_INET:
		{
			// This is where the buffer is created, allowing storage of the converted address
			char addr[INET_ADDRSTRLEN];

			// Start conversion of binary network format to presentation form
			ADDRTOSTR
			ADDRESS_FORMAT_STRING

			break;
		}

		case AF_INET6:
		{
			char addr[INET6_ADDRSTRLEN];
			internet_protocol_v6 = IFACE_ADDR_TYPE = 1; /* We use this inside of a helper function for generic logic */

			ADDRTOSTR
			ADDRESS_FORMAT_STRING

			break;
		}

		default:
		{
			printf("\t???? Address => \033[92;1m%p\033[0;m\n", &interface->addr);

			break;
		}
	}

	// Obtain NIC (Network Interface Card) netmask 
	return_interface_netmask(interface);

	return;
}
