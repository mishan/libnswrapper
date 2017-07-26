/* libnswrapper.so -- override name services-related system calls
 *
 * Copyright (C) 2014-2018 Misha Nasledov <misha@nasledov.com>
 */
#define _GNU_SOURCE

#include <features.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>

#if __GLIBC_MINOR__ >= 17
typedef int nameflag_t;
#else
typedef unsigned int nameflag_t;
#endif

// hook init_override_hosts to run before main()
void init_override_hosts (void) __attribute__((constructor));

typedef struct {
    char *lookup_host;
    char *replacement_host;
} host_overrides;

static host_overrides *override_hosts = NULL;
static int override_hosts_len = 0;

void init_override_hosts (void) {
    char *nswrapper_env;
    if ((nswrapper_env = getenv("NSWRAPPER_HOST")) != NULL) {
        // since we call strtok() on this and it's a pointer
        // to the variable in the environment, we must
        // work on a copy
        char *nswrap_config = strdup(nswrapper_env);
        // we also want to go through nswrap_config and count
        // the number of entries so we can allocate override_hosts
        // appropriately
        char *p = nswrap_config, *saveptr;
        int i, override_count = 1;

        // count number of comma separated entries
        while (*p != '\0') {
            if (*p == ',') {
                override_count++;
            }
            p++;
        }

        override_hosts = malloc(sizeof(host_overrides)*override_count);
        // use strtok_r() to populate this
        for (i = 0; ; nswrap_config = NULL) {
            char *nsconfig = strtok_r(nswrap_config, ",", &saveptr);
            if (nsconfig == NULL) {
                break;
            }
            p = strchr(nsconfig, ':');
            if (p == NULL) {
                continue;
            }
            *p++ = '\0'; // terminate and advance

            override_hosts[i].lookup_host = strdup(nsconfig);
            override_hosts[i].replacement_host = strdup(p);

            i++;
        }

        override_hosts_len = i;
        free(nswrap_config);
    } else {
        fprintf(stderr, "libnswrapper: No NSWRAPPER_HOST defined.");
    }
}

const char *get_host_override (const char *host) {
    int i;

    if (host == NULL) {
        /* NULL host is probably a programming error, but avoid segfaulting due
           to strcmp below. Pass the NULL on and let the original library
           function do the error handling, if any. */
        return host;
    }

    if (override_hosts == NULL) {
        init_override_hosts();
    }

    for (i = 0; i < override_hosts_len; i++) {
        if (!strcmp(host, override_hosts[i].lookup_host)) {
            return override_hosts[i].replacement_host;
        }
    }

    return host;
}

struct hostent *gethostbyname(const char *name)
{
    static struct hostent *(*original_gethostbyname)(const char *) = NULL;

    if (original_gethostbyname == NULL) {
        original_gethostbyname = dlsym(RTLD_NEXT, "gethostbyname");
    }

    return (*original_gethostbyname)(get_host_override(name));
}

int getaddrinfo(const char *node, const char *service,
                const struct addrinfo *hints,
                struct addrinfo **res)
{
    static int (*original_getaddrinfo)(const char *, const char *, const struct addrinfo *, struct addrinfo **) = NULL;

    if (original_getaddrinfo == NULL) {
        original_getaddrinfo = dlsym(RTLD_NEXT, "getaddrinfo");
    }

    return (*original_getaddrinfo)(get_host_override(node), service, hints, res);
}

int getnameinfo(const struct sockaddr *sa, socklen_t salen,
                char *host, socklen_t hostlen,
                char *serv, socklen_t servlen, nameflag_t flags)
{
    static int (*original_getnameinfo)(const struct sockaddr *, socklen_t, char *, size_t, char *, size_t, int) = NULL;
    const char *newhostconst = get_host_override(host);
    char *newhost = NULL;
    socklen_t newhostlen = -1;
    int ret = -1;

    if (original_getnameinfo == NULL) {
        original_getnameinfo = dlsym(RTLD_NEXT, "getnameinfo");
    }

    if (!strcmp(newhostconst, host)) {
        return (*original_getnameinfo)(sa, salen, host, hostlen, serv, servlen, flags);
    }

    newhostlen = strlen(newhostconst);
    // de-const the override by duplicating the string
    newhost = strndup(newhostconst, newhostlen);

    ret = (*original_getnameinfo)(sa, salen, newhost, newhostlen, serv, servlen, flags);
    free(newhost);
    return ret;
}

struct hostent *gethostbyname2(const char *name, int af)
{
    static struct hostent *(*original_gethostbyname2)(const char *, int) = NULL;

    if (original_gethostbyname2 == NULL) {
        original_gethostbyname2 = dlsym(RTLD_NEXT, "gethostbyname2");
    }

    return (*original_gethostbyname2)(get_host_override(name), af);
}

int gethostbyname_r(const char *name, struct hostent *ret, char *buf, size_t buflen, struct hostent **result, int *h_errnop)
{
    static int (*original_gethostbyname_r)(const char *name, struct hostent *ret, char *buf, size_t buflen, struct hostent **result, int *h_errnop) = NULL;

    if (original_gethostbyname_r == NULL) {
        original_gethostbyname_r = dlsym(RTLD_NEXT, "gethostbyname_r");
    }

    return (*original_gethostbyname_r)(get_host_override(name), ret, buf, buflen, result, h_errnop);
}

int gethostbyname2_r(const char *name, int af, struct hostent *ret, char *buf, size_t buflen, struct hostent **result, int *h_errnop)
{
    static int (*original_gethostbyname2_r)(const char *name, int af, struct hostent *ret, char *buf, size_t buflen, struct hostent **result, int *h_errnop) = NULL;

    if (original_gethostbyname2_r == NULL) {
        original_gethostbyname2_r = dlsym(RTLD_NEXT, "gethostbyname2_r");
    }

    return (*original_gethostbyname2_r)(get_host_override(name), af, ret, buf, buflen, result, h_errnop);
}
