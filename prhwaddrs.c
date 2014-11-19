#include    "get_hw_addrs.h"

static char *ntop(struct sockaddr *ip) {
    static char ip_str[64] = {'\0'};
    struct sockaddr_in *in4 = (struct sockaddr_in *)ip;
    struct sockaddr_in6 *in6 = (struct sockaddr_in6 *)ip;

    if(in4->sin_family == AF_INET) {
        inet_ntop(AF_INET, &in4->sin_addr, ip_str, sizeof(ip_str));
    } else if(in4->sin_family == AF_INET6) {
        inet_ntop(AF_INET6, &in6->sin6_addr, ip_str, sizeof(ip_str));
    } else {
        error("Invalid sin_family for ntop.\n");
    }
    return ip_str;
}

int main (int argc, char **argv) {
    struct hwa_info *hwa, *hwahead;
    struct sockaddr *sa;
    char *ptr;
    int i, prflag;

    printf("\n");

    for (hwahead = hwa = get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next) {
        
        printf("%s :%s", hwa->if_name, ((hwa->ip_alias) == IP_ALIAS) ? " (alias)\n" : "\n");
        
        if ( (sa = hwa->ip_addr) != NULL)
            printf("         IP addr = %s\n", ntop(sa));
                
        prflag = 0;
        i = 0;
        do {
            if (hwa->if_haddr[i] != '\0') {
                prflag = 1;
                break;
            }
        } while (++i < IFHWADDRLEN);

        if (prflag) {
            printf("         HW addr = ");
            ptr = hwa->if_haddr;
            i = IFHWADDRLEN;
            do {
                printf("%.2x%s", *ptr++ & 0xff, (i == 1) ? " " : ":");
            } while (--i > 0);
        }

        printf("\n         interface index = %d\n\n", hwa->if_index);
    }

    free_hwa_info(hwahead);
    exit(0);
}
