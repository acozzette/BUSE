/*
 * busemt - block-device userspace extensions
 * - EXPERIMENTAL MULTITHREADED VERSION
 * Copyright (C) 2013 Adam Cozzette
 *
 * This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <signal.h>
#include <poll.h>
#include "buse.h"

#if !defined(likely)
#if defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__clang__)
#define likely(x)       __builtin_expect(!!(x),1)
#define unlikely(x)     __builtin_expect(!!(x),0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif
#endif

void* emalloc(size_t size) {
	void* ret = malloc(size);
	if (unlikely(size && !ret)) {
		fprintf(stderr, "malloc failed to allocate %zu bytes. terminating...",
				size);
#if defined(__GLIBC__)
		//add glibc backtrace() stuff here?
#endif
		exit(EXIT_FAILURE);
	}
	return ret;
}
void* erealloc(void* ptr, size_t size) {
	void* ret = realloc(ptr, size);
	if (unlikely(size && !ret)) {
		fprintf(stderr, "realloc failed to allocate %zu bytes. terminating...",
				size);
#if defined(__GLIBC__)
		//add glibc backtrace() stuff here?
#endif
		exit(EXIT_FAILURE);
	}
	return ret;
}
void* ecalloc(size_t num, size_t size) {
	//im not implementing arbitrary precision integer logic for the error message, feel free to fix it
	void* ret = calloc(num, size);
	if (unlikely(num > 0 && size > 0 && !ret)) {
		fprintf(stderr, "calloc failed to allocate %zu bytes. terminating...",
				num * size);
#if defined(__GLIBC__)
		//add glibc backtrace() stuff here?
#endif
		exit(EXIT_FAILURE);

	}
	return ret;
}

/*
 * These helper functions were taken from cliserv.h in the nbd distribution.
 */
#ifdef WORDS_BIGENDIAN
u_int64_t ntohll(u_int64_t a) {
	return a;
}
#else
u_int64_t ntohll(u_int64_t a) {
	u_int32_t lo = a & 0xffffffff;
	u_int32_t hi = a >> 32U;
	lo = ntohl(lo);
	hi = ntohl(hi);
	return ((u_int64_t) lo) << 32U | hi;
}
#endif
#define htonll ntohll
pthread_mutex_t write_all_mutex1;
pthread_mutex_t write_all_mutex2;

static int read_all(int fd, char* buf, size_t count) {
	int bytes_read;
	while (count > 0) {
		bytes_read = read(fd, buf, count);
		assert(bytes_read > 0);
		buf += bytes_read;
		count -= bytes_read;
	}
	assert(count == 0);

	return 0;
}

static int write_all(int fd, char* buf, size_t count) {
	if (0 != pthread_mutex_lock(&write_all_mutex1)) {
		fprintf(stderr, "FAILED TO LOCK write_all_mutex1!!  TERMINATING");
		exit(EXIT_FAILURE);
	}
	//fprintf(stderr, "sending packet of size %zu: ", count);
	//fwrite(buf, count, 1, stderr);
	//fprintf(stderr, "\n\n\n\n");
	int bytes_written;
	while (count > 0) {
		bytes_written = write(fd, buf, count);
		assert(bytes_written > 0);
		buf += bytes_written;
		count -= bytes_written;
	}
	assert(count == 0);
	if (0 != pthread_mutex_unlock(&write_all_mutex1)) {
		fprintf(stderr, "FAILED TO UNLOCK write_all_mutex1!!  TERMINATING");
		exit(EXIT_FAILURE);
	}
	return 0;
}

struct buse_operations *aop_global;
void* userdata_global;
int sk_global;
struct nbd_reply replyTemplate;
pthread_mutex_t retardmutex;
pthread_mutex_t retardmutex2;

void* pthread_handle_request(void* requestcopy) {
	printf("thread %zu started\n", pthread_self());
//	if (0 != pthread_mutex_lock(&retardmutex)) {
//		fprintf(stderr, "FAILED TO LOCK RETARDMUTEX!!  TERMINATING");
//		exit(EXIT_FAILURE);
//	}
	printf("thread %zu executing\n", pthread_self());
	struct nbd_request request = *((struct nbd_request*) requestcopy);
	if (ntohl(request.type) != NBD_CMD_WRITE) {
		free(requestcopy);
	}
	struct nbd_reply reply = replyTemplate;
	u_int32_t len;
	u_int64_t from;
	char *chunk;
	{
		memcpy(reply.handle, request.handle, sizeof(reply.handle));
		reply.error = htonl(0);

		len = ntohl(request.len);
		from = ntohll(request.from);
		assert(request.magic == htonl(NBD_REQUEST_MAGIC));
		switch (ntohl(request.type)) {
		/* I may at some point need to deal with the the fact that the
		 * official nbd server has a maximum buffer size, and divides up
		 * oversized requests into multiple pieces. This applies to reads
		 * and writes.
		 */
		case NBD_CMD_READ:
			fprintf(stderr, "Request for read of size %d\n", len);
			/* Fill with zero in case actual read is not implemented */
			chunk = emalloc(sizeof(reply) + (len));
			if (aop_global->read) {
				reply.error = aop_global->read(&chunk[sizeof(reply)], len, from,
						userdata_global);
			} else {
				/* If user not specified read operation, return EPERM error */
				reply.error = htonl(EPERM);
			}

			memcpy(chunk, &reply, sizeof(reply));
			write_all(sk_global, (char*) chunk, sizeof(reply) + len);
			//write_all(sk_global, (char*) &reply, sizeof(struct nbd_reply));
			//write_all(sk_global, (char*) chunk, len);
			free(chunk);
			break;
		case NBD_CMD_WRITE:
			fprintf(stderr, "Request for write of size %d\n", len);
			//chunk = emalloc(len);
			//read_all(sk_global, chunk, len);
			if (aop_global->write) {
				reply.error = aop_global->write(
				/*chunk*/&(((char*) requestcopy)[sizeof(request)]), len, from,
						userdata_global);
			} else {
				/* If user not specified write operation, return EPERM error */
				reply.error = htonl(EPERM);
			}
			free(requestcopy);
			//free(chunk);
			write_all(sk_global, (char*) &reply, sizeof(reply));
			break;
		case NBD_CMD_DISC:
			/* Handle a disconnect request. */
			if (aop_global->disc) {
				aop_global->disc(userdata_global);
			}
			return 0;
#ifdef NBD_FLAG_SEND_FLUSH
		case NBD_CMD_FLUSH:
			if (aop_global->flush) {
				reply.error = aop_global->flush(userdata_global);
			}
			write_all(sk_global, (char*) &reply, sizeof(reply));
			break;
#endif
#ifdef NBD_FLAG_SEND_TRIM
		case NBD_CMD_TRIM:
			if (aop_global->trim) {
				reply.error = aop_global->trim(from, len, userdata_global);
			}
			write_all(sk_global, (char*) &reply, sizeof(reply));
			break;
#endif
		default:
			assert(0);
		}
	}
//	if (0 != pthread_mutex_unlock(&retardmutex)) {
//		fprintf(stderr, "FAILED TO UNLOCK RETARDMUTEX!!  TERMINATING");
//		exit(EXIT_FAILURE);
//	}
	return NULL;
}

int buse_main(const char* dev_file, const struct buse_operations *aop,
		void *userdata) {
	{
		replyTemplate.magic = htonl(NBD_REPLY_MAGIC);
		replyTemplate.error = htonl(0);
	}
	{
		pthread_mutex_init(&retardmutex, NULL);
		pthread_mutex_init(&retardmutex2, NULL);
		pthread_mutex_init(&write_all_mutex1, NULL);
		pthread_mutex_init(&write_all_mutex2, NULL);
	}
	userdata_global = userdata;
	aop_global = (struct buse_operations*) aop;
	int sp[2];
	int nbd, sk, err, tmp_fd;
	u_int64_t from;
	u_int32_t len;
	ssize_t bytes_read;
	struct nbd_request request;
	struct nbd_reply reply;
	char *chunk;

	err = socketpair(AF_UNIX, SOCK_STREAM, 0, sp);
	assert(!err);

	nbd = open(dev_file, O_RDWR);
	if (unlikely(nbd == -1)) {
		fprintf(stderr, "Failed to open `%s': %s\n"
				"Is kernel module `nbd' is loaded and you have permissions "
				"to access the device?\n", dev_file, strerror(errno));
		return 1;
	}

	err = ioctl(nbd, NBD_SET_SIZE, aop->size);
	assert(err != -1);
	err = ioctl(nbd, NBD_CLEAR_SOCK);
	assert(err != -1);

	if (!fork()) {
		/* The child needs to continue setting things up. */
		close(sp[0]);
		sk = sp[1];

		if (ioctl(nbd, NBD_SET_SOCK, sk) == -1) {
			fprintf(stderr, "ioctl(nbd, NBD_SET_SOCK, sk) failed.[%s]\n",
					strerror(errno));
		}
#if defined NBD_SET_FLAGS && defined NBD_FLAG_SEND_TRIM
		else if (ioctl(nbd, NBD_SET_FLAGS, NBD_FLAG_SEND_TRIM) == -1) {
			fprintf(stderr,
					"ioctl(nbd, NBD_SET_FLAGS, NBD_FLAG_SEND_TRIM) failed.[%s]\n",
					strerror(errno));
		}
#endif
		else {
			err = ioctl(nbd, NBD_DO_IT);
			fprintf(stderr, "nbd device terminated with code %d\n", err);
			if (err == -1)
				fprintf(stderr, "%s\n", strerror(errno));
		}

		ioctl(nbd, NBD_CLEAR_QUE);
		ioctl(nbd, NBD_CLEAR_SOCK);

		exit(0);
	}

	/* The parent opens the device file at least once, to make sure the
	 * partition table is updated. Then it closes it and starts serving up
	 * requests. */

	tmp_fd = open(dev_file, O_RDONLY);
	assert(tmp_fd != -1);
	close(tmp_fd);

	close(sp[1]);
	sk_global = sk = sp[0];

	reply.magic = htonl(NBD_REPLY_MAGIC);
	reply.error = htonl(0);

	if (1) {
		printf("threading mode: pthreads\n");
		while ((bytes_read = read(sk, &request, sizeof(request))) > 0) {
			{
				if (unlikely(bytes_read != sizeof(request))) {
					fprintf(stderr,
							"ERROR FROM NBD SERVER! REQUESTS SHOULD AWAYS BE OF SIZE %zu, BUT GOT A REQUEST WITH RQ SIZE %zd... TERMINATING. REQUEST COPY FOLLOWS:",
							sizeof(request), bytes_read);
					fwrite(&request, bytes_read, 1, stderr);
					exit(EXIT_FAILURE);
				}

				if (unlikely(request.magic != htonl(NBD_REQUEST_MAGIC))) {
					fprintf(stderr,
							"GOT SOMETHING OTHER THAN NBD MAGIC REQUEST... TERMINATING. REQUEST COPY FOLLOWS:");
					fwrite(&request, bytes_read, 1, stderr);
					exit(EXIT_FAILURE);
				}
				struct nbd_request* requestcopy;
				if (ntohl(request.type) == NBD_CMD_WRITE) {
					uint32_t len = ntohl(request.len);
					requestcopy = emalloc(sizeof(request) + len);
					(*requestcopy) = request;
					read_all(sk, &(((char*) requestcopy)[sizeof(request)]),
							len);
				} else {
					requestcopy = emalloc(sizeof(request));
					(*requestcopy) = request;
				}
				pthread_t newthread;
				while (true) {
					int ret = pthread_create(&newthread, NULL,
							pthread_handle_request, requestcopy);
					if (ret == 0) {
						//pthread_join(newthread, NULL);
						break;
					} else {
						fprintf(stderr,
								"Error: failed to create new thread. errori: %i, strerror: %s. will sleep 1 second and try again..",
								ret, strerror(ret));
						sleep(1);
					}
				}
			}
		}
	} else if (0) {
		//WARNING: DOES NOT WORK
		printf("treading mode: fork\n");
		while ((bytes_read = read(sk, &request, sizeof(request))) > 0) {
			pid_t childPID;
			childPID = fork();
			if (childPID < 0) {
				fprintf(stderr, "ERROR FORK FAILED");
				exit(EXIT_FAILURE);
			}
			if (childPID != 0) {
				//parent
			} else {
				//child.
				assert(bytes_read == sizeof(request));
				memcpy(reply.handle, request.handle, sizeof(reply.handle));
				reply.error = htonl(0);

				len = ntohl(request.len);
				from = ntohll(request.from);
				assert(request.magic == htonl(NBD_REQUEST_MAGIC));

				switch (ntohl(request.type)) {
				/* I may at some point need to deal with the the fact that the
				 * official nbd server has a maximum buffer size, and divides up
				 * oversized requests into multiple pieces. This applies to reads
				 * and writes.
				 */
				case NBD_CMD_READ:
					fprintf(stderr, "Request for read of size %d\n", len);
					/* Fill with zero in case actual read is not implemented */

					chunk = emalloc(sizeof(reply) + (len));
					if (aop->read) {
						reply.error = aop->read(&chunk[sizeof(reply)], len,
								from, userdata_global);
					} else {
						/* If user not specified read operation, return EPERM error */
						reply.error = htonl(EPERM);
					}
					memcpy(chunk, &reply, sizeof(reply));
					write_all(sk_global, (char*) chunk,
							sizeof(struct nbd_reply) + len);
					//write_all(sk, (char*) &reply, sizeof(struct nbd_reply));
					//write_all(sk, (char*) chunk, len);
					free(chunk);
					break;
				case NBD_CMD_WRITE:
					fprintf(stderr, "Request for write of size %d\n", len);
					chunk = emalloc(len);
					read_all(sk, chunk, len);
					if (aop->write) {
						reply.error = aop->write(chunk, len, from, userdata);
					} else {
						/* If user not specified write operation, return EPERM error */
						reply.error = htonl(EPERM);
					}
					free(chunk);
					write_all(sk, (char*) &reply, sizeof(struct nbd_reply));
					break;
				case NBD_CMD_DISC:
					/* Handle a disconnect request. */
					if (aop->disc) {
						aop->disc(userdata);
					}
					return 0;
#ifdef NBD_FLAG_SEND_FLUSH
				case NBD_CMD_FLUSH:
					if (aop->flush) {
						reply.error = aop->flush(userdata);
					}
					write_all(sk, (char*) &reply, sizeof(struct nbd_reply));
					break;
#endif
#ifdef NBD_FLAG_SEND_TRIM
				case NBD_CMD_TRIM:
					if (aop->trim) {
						reply.error = aop->trim(from, len, userdata);
					}
					write_all(sk, (char*) &reply, sizeof(struct nbd_reply));
					break;
#endif
				default:
					assert(0);
				}
				return EXIT_SUCCESS;
			}
		}
	} else {
		printf("threading mode: single thread\n");
		while ((bytes_read = read(sk, &request, sizeof(request))) > 0) {
			assert(bytes_read == sizeof(request));
			memcpy(reply.handle, request.handle, sizeof(reply.handle));
			reply.error = htonl(0);

			len = ntohl(request.len);
			from = ntohll(request.from);
			assert(request.magic == htonl(NBD_REQUEST_MAGIC));

			switch (ntohl(request.type)) {
			/* I may at some point need to deal with the the fact that the
			 * official nbd server has a maximum buffer size, and divides up
			 * oversized requests into multiple pieces. This applies to reads
			 * and writes.
			 */
			case NBD_CMD_READ:
				fprintf(stderr, "Request for read of size %d\n", len);
				/* Fill with zero in case actual read is not implemented */
//				chunk = emalloc(len);
//				if (aop->read) {
//					reply.error = aop->read(chunk, len, from, userdata);
//				} else {
//					/* If user not specified read operation, return EPERM error */
//					reply.error = htonl(EPERM);
//				}
//				write_all(sk, (char*) &reply, sizeof(struct nbd_reply));
//				write_all(sk, (char*) chunk, len);
				chunk = emalloc(sizeof(reply) + (len));
				if (aop_global->read) {
					reply.error = aop_global->read(&chunk[sizeof(reply)], len,
							from, userdata_global);
				} else {
					/* If user not specified read operation, return EPERM error */
					reply.error = htonl(EPERM);
				}

				memcpy(chunk, &reply, sizeof(reply));
				write_all(sk_global, (char*) chunk,
						sizeof(struct nbd_reply) + len);

				free(chunk);
				break;
			case NBD_CMD_WRITE:
				fprintf(stderr, "Request for write of size %d\n", len);
				chunk = emalloc(len);
				read_all(sk, chunk, len);
				if (aop->write) {
					reply.error = aop->write(chunk, len, from, userdata);
				} else {
					/* If user not specified write operation, return EPERM error */
					reply.error = htonl(EPERM);
				}
				free(chunk);
				write_all(sk, (char*) &reply, sizeof(struct nbd_reply));
				break;
			case NBD_CMD_DISC:
				/* Handle a disconnect request. */
				if (aop->disc) {
					aop->disc(userdata);
				}
				return 0;
#ifdef NBD_FLAG_SEND_FLUSH
			case NBD_CMD_FLUSH:
				if (aop->flush) {
					reply.error = aop->flush(userdata);
				}
				write_all(sk, (char*) &reply, sizeof(struct nbd_reply));
				break;
#endif
#ifdef NBD_FLAG_SEND_TRIM
			case NBD_CMD_TRIM:
				if (aop->trim) {
					reply.error = aop->trim(from, len, userdata);
				}
				write_all(sk, (char*) &reply, sizeof(struct nbd_reply));
				break;
#endif
			default:
				assert(0);
			}
		}
	}
	if (bytes_read == -1)
		fprintf(stderr, "%s\n", strerror(errno));
	return 0;
}
