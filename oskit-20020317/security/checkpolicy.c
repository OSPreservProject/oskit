/* FLASK */

/*
 * Copyright (c) 1999, 2000 The University of Utah and the Flux Group.
 * All rights reserved.
 * 
 * Contributed by the Computer Security Research division,
 * INFOSEC Research and Technology Office, NSA.
 * 
 * This file is part of the Flux OSKit.  The OSKit is free software, also known
 * as "open source;" you can redistribute it and/or modify it under the terms
 * of the GNU General Public License (GPL), version 2, as published by the Free
 * Software Foundation (FSF).  To explore alternate licensing terms, contact
 * the University of Utah at csl-dist@cs.utah.edu or +1-801-585-3271.
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */

#ifndef __KERNEL__

/* 
 * checkpolicy
 *
 * Load and check a policy configuration.
 *
 * A policy configuration is created in a text format,
 * and then compiled into a binary format for use by
 * the security server.  By default, checkpolicy reads
 * the text format.   If '-b' is specified, then checkpolicy
 * reads the binary format instead.
 * 
 * If '-o output_file' is specified, then checkpolicy 
 * writes the binary format version of the configuration
 * to the specified output file.  
 * 
 * If '-d' is specified, then checkpolicy permits the user 
 * to interactively test the security server functions with 
 * the loaded policy configuration.
 */

#include "policydb.h"
#include "services.h"
#include "queue.h"

extern char *optarg;
extern int optind;

extern policydb_t *policydbp;
extern queue_t id_queue;
extern unsigned int policydb_errors;

extern FILE *yyin;
extern int yyparse(void);

char *txtfile = "policy.conf";
char *binfile = "policy";

void usage(char *progname)
{
	printf("usage:  %s [-b] [-d] [-o output_file] [input_file]\n", progname);
	exit(1);
}

static int print_sid(security_id_t sid,
		     context_struct_t * context, void *data)
{
	security_context_t scontext;
	unsigned int scontext_len;
	int rc;

	rc = security_sid_to_context(sid, &scontext, &scontext_len);
	if (rc)
		printf("sid %d -> error %d\n", sid, rc);
	else {
		printf("sid %d -> scontext %s\n", sid, scontext);
		free(scontext);
	}
	return 0;
}

static int find_perm(hashtab_key_t key, hashtab_datum_t datum, void *p)
{
	unsigned int *valuep;
	perm_datum_t *perdatum;

	valuep = (unsigned int *) p;
	perdatum = (perm_datum_t *) datum;

	if (*valuep == perdatum->value)
		return (int) key;

	return 0;
}

int main(int argc, char **argv)
{
	security_class_t tclass;
	security_id_t ssid, tsid;
	security_context_t scontext;
	access_vector_t allowed, decided, auditallow, auditdeny, notify;
	class_datum_t *cladatum;
	char ans[80 + 1], *perm, *file = txtfile, *outfile = NULL;
	unsigned int scontext_len, pathlen, seqno, i;
	unsigned int protocol, port, addr;
	unsigned int trans = 0, binary = 0, debug = 0;
	int ret, ch;
	FILE *fp, *outfp = NULL;


	while ((ch = getopt(argc, argv, "o:db")) != EOF) {
		switch (ch) {
		case 'o':
			outfile = optarg;
			break;
		case 'b':
			binary = 1;
			file = binfile;
			break;
		case 'd':
			debug = 1;
			break;
		default:
			usage(argv[0]);
		}
	}

	if (optind != argc) {
		file = argv[optind++];
		if (optind != argc)
			usage(argv[0]);
	}
	printf("%s:  loading policy configuration from %s\n", argv[0],
	       file);

	yyin = fopen(file, "r");
	if (!yyin) {
		fprintf(stderr, "%s:  unable to open %s\n", argv[0], 
			file);
		exit(1);
	}
	if (binary) {
		if (policydb_read(&policydb, yyin)) {
			fprintf(stderr, "%s:  error(s) encountered while parsing configuration\n", argv[0]);
			exit(1);
		}
	} else {
		if (policydb_init(&policydb))
			exit(1);

		id_queue = queue_create();
		if (!id_queue) {
			fprintf(stderr, "%s:  out of memory\n", argv[0]);
			exit(1);
		}
		policydbp = &policydb;
		policydb_errors = 0;
		if (yyparse() || policydb_errors) {
			fprintf(stderr, "%s:  error(s) encountered while parsing configuration\n", argv[0]);
			exit(1);
		}
		queue_destroy(id_queue);
	}
	fclose(yyin);

	if (policydb_load_isids(&policydb, &sidtab))
		exit(1);

	printf("%s:  policy configuration loaded\n", argv[0]);

	if (outfile) {
		printf("%s:  writing binary representation to %s\n",
		       argv[0], outfile);
		outfp = fopen(outfile, "w");
		if (!outfp) {
			perror(outfile);
			exit(1);
		}
		ret = policydb_write(&policydb, outfp);
		if (ret) {
			fprintf(stderr, "%s:  error writing %s\n",
				argv[0], outfile);
			exit(1);
		}
	}
	if (!debug)
		exit(0);

	ss_initialized = 1;

      menu:
	printf("\nSelect an option:\n");
	printf("0)  Call compute_access_vector\n");
	printf("1)  Call sid_to_context\n");
	printf("2)  Call context_to_sid\n");
	printf("3)  Call transition_sid\n");
	printf("4)  Call member_sid\n");
	printf("5)  Call list_sids\n");
	printf("6)  Call load_policy\n");
	printf("7)  Call fs_sid\n");
	printf("8)  Call port_sid\n");
	printf("9)  Call netif_sid\n");
	printf("a)  Call node_sid\n");
	printf("m)  Show menu again\n");
	printf("q)  Exit\n");
	while (1) {
		printf("\nChoose:  ");
		fgets(ans, sizeof(ans), stdin);
		trans = 0;
		switch (ans[0]) {
		case '0':
			printf("source sid?  ");
			fgets(ans, sizeof(ans), stdin);
			ssid = atoi(ans);

			printf("target sid?  ");
			fgets(ans, sizeof(ans), stdin);
			tsid = atoi(ans);

			printf("target class?  ");
			fgets(ans, sizeof(ans), stdin);
			if (isdigit(ans[0])) {
				tclass = atoi(ans);
				if (!tclass || tclass > policydb.p_classes.nprim) {
					printf("\nNo such class.\n");
					break;
				}
				cladatum = policydb.class_val_to_struct[tclass - 1];
			} else {
				ans[strlen(ans) - 1] = 0;
				cladatum = (class_datum_t *) hashtab_search(policydb.p_classes.table,
								    ans);
				if (!cladatum) {
					printf("\nNo such class\n");
					break;
				}
				tclass = cladatum->value;
			}

			if (!cladatum->comdatum && !cladatum->permissions.nprim) {
				printf("\nNo access vector definition for that class\n");
				break;
			}
			ret = security_compute_av(ssid, tsid, tclass, 0,
						  &allowed, &decided,
#ifdef CONFIG_FLASK_AUDIT
						  &auditallow, &auditdeny,
#endif
#ifdef CONFIG_FLASK_NOTIFY
						  &notify,
#endif
						  &seqno);
			switch (ret) {
			case 0:
				printf("\nallowed {");
				for (i = 1; i <= sizeof(allowed) * 8; i++) {
					if (allowed & (1 << (i - 1))) {
						perm = (char *) hashtab_map(cladatum->permissions.table,
							  find_perm, &i);

						if (!perm && cladatum->comdatum) {
							perm = (char *) hashtab_map(cladatum->comdatum->permissions.table,
							  find_perm, &i);
						}
						if (perm)
							printf(" %s", perm);
					}
				}
				printf(" }\n");
				break;
			case -EINVAL:
				printf("\ninvalid sid\n");
				break;
			default:
				printf("return code 0x%x\n", ret);
			}
			break;
		case '1':
			printf("sid?  ");
			fgets(ans, sizeof(ans), stdin);
			ssid = atoi(ans);
			ret = security_sid_to_context(ssid,
					       &scontext, &scontext_len);
			switch (ret) {
			case 0:
				printf("\nscontext %s\n", scontext);
				free(scontext);
				break;
			case -EINVAL:
				printf("\ninvalid sid\n");
				break;
			case -ENOMEM:
				printf("\nout of memory\n");
				break;
			default:
				printf("return code 0x%x\n", ret);
			}
			break;
		case '2':
			printf("scontext?  ");
			fgets(ans, sizeof(ans), stdin);
			scontext_len = strlen(ans);
			ans[scontext_len - 1] = 0;
			ret = security_context_to_sid(ans, scontext_len,
						      &ssid);
			switch (ret) {
			case 0:
				printf("\nsid %d\n", ssid);
				break;
			case -EINVAL:
				printf("\ninvalid context\n");
				break;
			case -ENOMEM:
				printf("\nout of memory\n");
				break;
			default:
				printf("return code 0x%x\n", ret);
			}
			break;
		case '3':
			trans = 1;
		case '4':
			printf("source sid?  ");
			fgets(ans, sizeof(ans), stdin);
			ssid = atoi(ans);
			printf("target sid?  ");
			fgets(ans, sizeof(ans), stdin);
			tsid = atoi(ans);

			printf("object class?  ");
			fgets(ans, sizeof(ans), stdin);
			if (isdigit(ans[0])) {
				tclass = atoi(ans);
				if (!tclass || tclass > policydb.p_classes.nprim) {
					printf("\nNo such class.\n");
					break;
				}
			} else {
				ans[strlen(ans) - 1] = 0;
				cladatum = (class_datum_t *) hashtab_search(policydb.p_classes.table,
								    ans);
				if (!cladatum) {
					printf("\nNo such class\n");
					break;
				}
				tclass = cladatum->value;
			}

			if (trans)
				ret = security_transition_sid(ssid, tsid, tclass, &ssid);
			else
				ret = security_member_sid(ssid, tsid, tclass, &ssid);
			switch (ret) {
			case 0:
				printf("\nsid %d\n", ssid);
				break;
			case -EINVAL:
				printf("\ninvalid sid\n");
				break;
			case -ENOMEM:
				printf("\nout of memory\n");
				break;
			default:
				printf("return code 0x%x\n", ret);
			}
			break;
		case '5':
			sidtab_map(&sidtab, print_sid, 0);
			break;
		case '6':
			printf("pathname?  ");
			fgets(ans, sizeof(ans), stdin);
			pathlen = strlen(ans);
			ans[pathlen - 1] = 0;
			printf("%s:  loading policy configuration from %s\n", argv[0], ans);
			fp = fopen(ans, "r");
			if (!fp) {
				printf("%s:  unable to open %s\n", argv[0], ans);
				break;
			}
			ret = security_load_policy(fp);
			switch (ret) {
			case 0:
				printf("\nsuccess\n");
				break;
			case -EINVAL:
				printf("\ninvalid policy\n");
				break;
			case -ENOMEM:
				printf("\nout of memory\n");
				break;
			default:
				printf("return code 0x%x\n", ret);
			}
			fclose(fp);
			break;
		case '7':
			printf("fs kdevname?  ");
			fgets(ans, sizeof(ans), stdin);
			ans[strlen(ans) - 1] = 0;
			security_fs_sid(ans, &ssid, &tsid);
			printf("fs_sid %d default_file_sid %d\n",
			       ssid, tsid);
			break;
		case '8':
			printf("protocol?  ");
			fgets(ans, sizeof(ans), stdin);
			ans[strlen(ans) - 1] = 0;
			if (!strcmp(ans, "tcp") || !strcmp(ans, "TCP"))
				protocol = IPPROTO_TCP;
			else if (!strcmp(ans, "udp") || !strcmp(ans, "UDP"))
				protocol = IPPROTO_UDP;
			else {
				printf("unknown protocol\n");
				break;
			}
			printf("port? ");
			fgets(ans, sizeof(ans), stdin);
			port = atoi(ans);
			security_port_sid(0, 0, protocol, port, &ssid);
			printf("sid %d\n", ssid);
			break;
		case '9':
			printf("netif name?  ");
			fgets(ans, sizeof(ans), stdin);
			ans[strlen(ans) - 1] = 0;
			security_netif_sid(ans, &ssid, &tsid);
			printf("if_sid %d default_msg_sid %d\n",
			       ssid, tsid);
			break;
		case 'a':
			printf("node address?  ");
			fgets(ans, sizeof(ans), stdin);
			ans[strlen(ans) - 1] = 0;
			addr = inet_addr(ans);
			security_node_sid(AF_INET, &addr, sizeof addr, &ssid);
			printf("sid %d\n", ssid);
			break;
		case 'm':
			goto menu;
		case 'q':
			exit(0);
			break;
		default:
			printf("\nUnknown option %s.\n", ans);
		}
	}

	return 0;
}

#endif

/* FLASK */
