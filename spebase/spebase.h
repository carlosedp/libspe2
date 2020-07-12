/*
 * libspe2 - A wrapper library to adapt the JSRE SPU usage model to SPUFS 
 * Copyright (C) 2005 IBM Corp. 
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by 
 * the Free Software Foundation; either version 2.1 of the License, 
 * or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful, but 
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 *  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public 
 *  License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public License 
 *   along with this library; if not, write to the Free Software Foundation, 
 *   Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
 /** \file
  * spebase.h contains the public API funtions
  */
#ifndef _spebase_h_
#define _spebase_h_

#ifdef __cplusplus
extern "C"
{
#endif

#include <pthread.h>

#include "libspe2-types.h"

#define __PRINTF(fmt, args...) { fprintf(stderr,fmt , ## args); }
#ifdef DEBUG
#define DEBUG_PRINTF(fmt, args...) __PRINTF(fmt , ## args)
#else
#define DEBUG_PRINTF(fmt, args...)
#endif

/** NOTE: NUM_MBOX_FDS must always be the last element in the enumeration */
enum fd_name {
	FD_MBOX,
	FD_MBOX_STAT,
	FD_IBOX,
	FD_IBOX_NB,
	FD_IBOX_STAT,
	FD_WBOX,
	FD_WBOX_NB,
	FD_WBOX_STAT,
	FD_SIG1,
	FD_SIG2,
	FD_MFC,
	FD_MSS,
	NUM_MBOX_FDS
};

/*
 * "Private" structure -- do no use, if you want to achieve binary compatibility
 */
struct spe_context_base_priv {
	/* mutex to protect members which modified in lifetime of the context:
	     spe_fds_array, entry
	 */
	pthread_mutex_t fd_lock[NUM_MBOX_FDS];

	/* SPE Group base directory fd */
	int		fd_grp_dir;
	
	/* SPE SPUFS base dir fd*/
	int fd_spe_dir;
	
	/* SPE Program execution and environment flags */
	unsigned int    flags;
	
	/* SPE Mailbox and Signal fds */
	int spe_fds_array[NUM_MBOX_FDS];
	int spe_fds_refcount[NUM_MBOX_FDS];
	
	/* event pipes for speevent library */
	int ev_pipe[2];
	
	/* Base Addresses of memory mapped SPE areas */
	void	*psmap_mmap_base;
	void	*mem_mmap_base;
	void	*mfc_mmap_base;
	void	*mssync_mmap_base;
	void	*cntl_mmap_base;
	void	*signal1_mmap_base;
	void	*signal2_mmap_base;
	
	/* SPE program entry point generated by elf_load() */
	int		entry;

	/* We need to keep the entry point for emulated isolated contexts,
	 * and ignore the value provided to spe_context_run */
	int		emulated_entry;
	
	/* This is used to keep track of tags used for proxy DMA operations
	 * so we can use the zero tagmask parameter in the status functions*/
	 
	 int active_tagmask;
};


/* spe related sizes
 */

#define LS_SIZE				0x40000   /* 256K (in bytes) */
#define PSMAP_SIZE			0x20000   /* 128K (in bytes) */
#define MFC_SIZE			0x1000
#define MSS_SIZE			0x1000
#define CNTL_SIZE			0x1000
#define SIGNAL_SIZE			0x1000

#define MSSYNC_OFFSET			0x00000
#define MFC_OFFSET			0x03000
#define CNTL_OFFSET			0x04000
#define SIGNAL1_OFFSET			0x14000
#define SIGNAL2_OFFSET			0x1c000

/**
 * Location of the PPE-assisted library call buffer
 * for emulated isolation contexts.
 */
#define SPE_EMULATE_PARAM_BUFFER	0x3e000

/* 
 */
#define SPE_PROGRAM_NORMAL_END		0x2000
#define SPE_PROGRAM_LIBRARY_CALL	0x2100

/**
 * Isolated exit codes: 0x220x
 */
#define SPE_PROGRAM_ISOLATED_STOP	0x2200
#define SPE_PROGRAM_ISO_LOAD_COMPLETE	0x2206

/**
 * spe_context: This holds the persistant information of a SPU instance
 *		it is created by spe_create_context()
*/

struct spe_gang_context_base_priv
{
	/* SPE Gang execution and environment flags */
	unsigned int    flags;

	/* SPE Mailbox and Signal fds */
	int fd_gang_dir;
	
	char gangname[256]; 
};


/* Function Prototypes
 */

/**
 * _base_spe_context_create creates a single SPE context, i.e., the corresponding directory
 * is created in SPUFS either as a subdirectory of a gang or
 * individually (maybe this is best considered a gang of one)
 * 
 * @param flags 
 * @param gctx specify NULL if not belonging to a gang
 * @param aff_spe specify NULL to skip affinity information
 */
extern spe_context_ptr_t _base_spe_context_create(unsigned int flags, spe_gang_context_ptr_t gctx, spe_context_ptr_t aff_spe);

/**
 * creates the directory in SPUFS that will contain all SPEs
 * that are considered a gang
 * Note: I would like to generalize this to a "group" or "set"
 *        Additional attributes maintained at the group level
 *        should be used to define scheduling constraints such
 *        "temporal" (e.g., scheduled all at the same time, i.e., a gang)
 *        "topology" (e.g., "closeness" of SPEs for optimal communication)
 */
extern spe_gang_context_ptr_t _base_spe_gang_context_create(unsigned int flags);

/**
 * _base_spe_program_load loads an ELF image into a context
 * 
 * @param spectx Specifies the SPE context
 * 
 * @param program handle to the ELF image
 */
extern int _base_spe_program_load(spe_context_ptr_t spectx, spe_program_handle_t *program);

/**
 * Check if the emulated loader is present in the filesystem
 * @return Non-zero if the loader is available, otherwise zero.
 */
int _base_spe_emulated_loader_present(void);

/**
 * _base_spe_context_destroy cleans up what is left when an SPE executable has exited. 
 * Closes open file handles and unmaps memory areas.
 * 
 * @param spectx Specifies the SPE context
 */
extern int _base_spe_context_destroy(spe_context_ptr_t spectx);

/**
 * _base_spe_gang_context_destroy destroys a gang context and frees associated resources
 *
 * @param gctx Specifies the SPE gang context
 */
extern int _base_spe_gang_context_destroy(spe_gang_context_ptr_t gctx);

/**
 * _base_spe_context_run starts execution of an SPE context with a loaded image
 *
 * @param spectx Specifies the SPE context
 *
 * @param entry entry point for the SPE programm. If set to 0, entry point is determined
 * by the ELF loader.
 *
 * @param runflags valid values are:\n
 *  SPE_RUN_USER_REGS Specifies that the SPE setup registers r3, r4, and r5 are initialized 
 *  with the 48 bytes pointed to by argp.\n
 *  SPE_NO_CALLBACKS do not use built in library functions.\n
 *  
 *
 * @param argp An (optional) pointer to application specific data, and is passed as the second 
 * parameter to the SPE program.
 * 
 * @param envp An (optional) pointer to environment specific data, and is passed as the third 
 * parameter to the SPE program.
 */
extern int _base_spe_context_run(spe_context_ptr_t spe, unsigned int *entry, 
					unsigned int runflags, void *argp, void *envp, spe_stop_info_t *stopinfo);

/**
 * _base_spe_image_close unmaps an SPE ELF object that was previously mapped using 
 * spe_open_image.
 * @param handle handle to open file
 * 
 * @retval 0 On success, spe_close_image returns 0. 
 * @retval -1 On failure, -1 is returned and errno is set appropriately.\n
 * Possible values for errno:\n
 * EINVAL From spe_close_image, this indicates that the file, specified by 
 * filename, was not previously mapped by a call to spe_open_image.
 */
extern int _base_spe_image_close(spe_program_handle_t *handle);

/**
 * _base_spe_image_open maps an SPE ELF executable indicated by filename into system 
 * memory and returns the mapped address appropriate for use by the spe_create_thread 
 * API. It is often more convenient/appropriate to use the loading methodologies 
 * where SPE ELF objects are converted to PPE static or shared libraries with 
 * symbols which point to the SPE ELF objects after these special libraries are 
 * loaded. These libraries are then linked with the associated PPE code to provide 
 * a direct symbol reference to the SPE ELF object. The symbols in this scheme 
 * are equivalent to the address returned from the spe_open_image function.
 * SPE ELF objects loaded using this function are not shared with other processes, 
 * but SPE ELF objects loaded using the other scheme, mentioned above, can be 
 * shared if so desired.
 * 
 * @param filename Specifies the filename of an SPE ELF executable to be loaded 
 * and mapped into system memory.
 * 
 * @return On success, spe_open_image returns the address at which the specified 
 * SPE ELF object has been mapped. On failure, NULL is returned and errno is set 
 * appropriately.\n
 * Possible values for errno include:\n
 * EACCES The calling process does not have permission to access the 
 * specified file.\n
 * EFAULT The filename parameter points to an address that was not 
 * contained in the calling process`s address space.
 * 
 * A number of other errno values could be returned by the open(2), fstat(2), 
 * mmap(2), munmap(2), or close(2) system calls which may be utilized by the 
 * spe_open_image or spe_close_image functions.
 * @sa spe_create_thread
 */
extern spe_program_handle_t *_base_spe_image_open(const char *filename);

/**
 * The _base_spe_mfcio_put function places a put DMA command on the proxy command queue 
 * of the SPE thread specified by speid. The put command transfers size bytes of 
 * data starting at the local store address specified by ls to the effective 
 * address specified by ea. The DMA is identified by the tag id specified by 
 * tag and performed according transfer class and replacement class specified 
 * by tid and rid respectively.
 * 
 * @param spectx Specifies the SPE context
 * @param ls Specifies the starting local store destination address.
 * @param ea Specifies the starting effective address source address.
 * @param size Specifies the size, in bytes, to be transferred.
 * @param tag Specifies the tag id used to identify the DMA command.
 * @param tid Specifies the transfer class identifier of the DMA command.
 * @param rid Specifies the replacement class identifier of the DMA command.
 * @return On success, return 0. On failure, -1 is returned.
 */
extern int _base_spe_mfcio_put(spe_context_ptr_t spectx, 
			unsigned int ls, 
			void *ea, 
			unsigned int size, 
			unsigned int tag, 
			unsigned int tid, 
			unsigned int rid);

/**
 * The _base_spe_mfcio_putb function is identical to _base_spe_mfcio_put except that it places a 
 * putb (put with barrier) DMA command on the proxy command queue. The barrier 
 * form ensures that this command and all sequence commands with the same tag 
 * identifier as this command are locally ordered with respect to all previously i
 * ssued commands with the same tag group and command queue.
 * 
 * @param spectx Specifies the SPE context
 * @param ls Specifies the starting local store destination address.
 * @param ea Specifies the starting effective address source address.
 * @param size Specifies the size, in bytes, to be transferred.
 * @param tag Specifies the tag id used to identify the DMA command.
 * @param tid Specifies the transfer class identifier of the DMA command.
 * @param rid Specifies the replacement class identifier of the DMA command.
 * @return On success, return 0. On failure, -1 is returned.
 */
int _base_spe_mfcio_putb(spe_context_ptr_t spectx, 
			unsigned int ls, 
			void *ea, 
			unsigned int size, 
			unsigned int tag, 
			unsigned int tid, 
			unsigned int rid);

/**
 * The _base_spe_mfcio_putf function is identical to _base_spe_mfcio_put except that it places a 
 * putf (put with fence) DMA command on the proxy command queue. The fence form 
 * ensures that this command is locally ordered with respect to all previously 
 * issued commands with the same tag group and command queue.
 * 
 * @param spectx Specifies the SPE context
 * @param ls Specifies the starting local store destination address.
 * @param ea Specifies the starting effective address source address.
 * @param size Specifies the size, in bytes, to be transferred.
 * @param tag Specifies the tag id used to identify the DMA command.
 * @param tid Specifies the transfer class identifier of the DMA command.
 * @param rid Specifies the replacement class identifier of the DMA command.
 * @return On success, return 0. On failure, -1 is returned.
 */
int _base_spe_mfcio_putf(spe_context_ptr_t spectx, 
			unsigned int ls, 
			void *ea, 
			unsigned int size, 
			unsigned int tag, 
			unsigned int tid, 
			unsigned int rid);

/**
 * The _base_spe_mfcio_get function places a get DMA command on the proxy command queue 
 * of the SPE thread specified by speid. The get command transfers size bytes of 
 * data starting at the effective address specified by ea to the local store 
 * address specified by ls. The DMA is identified by the tag id specified by tag 
 * and performed according to the transfer class and replacement class specified 
 * by tid and rid respectively.
 * @param spectx Specifies the SPE context
 * @param ls Specifies the starting local store destination address.
 * @param ea Specifies the starting effective address source address.
 * @param size Specifies the size, in bytes, to be transferred.
 * @param tag Specifies the tag id used to identify the DMA command.
 * @param tid Specifies the transfer class identifier of the DMA command.
 * @param rid Specifies the replacement class identifier of the DMA command.
 * @return On success, return 0. On failure, -1 is returned.
*/
int _base_spe_mfcio_get(spe_context_ptr_t spectx, 
			unsigned int ls, 
			void *ea, 
			unsigned int size, 
			unsigned int tag, 
			unsigned int tid, 
			unsigned int rid);

/**
 * The _base_spe_mfcio_getb function is identical to _base_spe_mfcio_get except that it places a 
 * getb (get with barrier) DMA command on the proxy command queue. The barrier 
 * form ensures that this command and all sequence commands with the same tag 
 * identifier as this command are locally ordered with respect to all previously 
 * issued commands with the same tag group and command queue.
 * 
 * @param spectx Specifies the SPE context
 * @param ls Specifies the starting local store destination address.
 * @param ea Specifies the starting effective address source address.
 * @param size Specifies the size, in bytes, to be transferred.
 * @param tag Specifies the tag id used to identify the DMA command.
 * @param tid Specifies the transfer class identifier of the DMA command.
 * @param rid Specifies the replacement class identifier of the DMA command.
 * @return On success, return 0. On failure, -1 is returned.
 */
int _base_spe_mfcio_getb(spe_context_ptr_t spectx, 
			unsigned int ls, 
			void *ea, 
			unsigned int size, 
			unsigned int tag, 
			unsigned int tid, 
			unsigned int rid);

/**
 * The _base_spe_mfcio_getf function is identical to _base_spe_mfcio_get except that it places a 
 * getf (get with fence) DMA command on the proxy command queue. The fence form 
 * ensure that this command is locally ordered with respect to all previously 
 * issued commands with the same tag group and command queue.
 * 
 * @param spectx Specifies the SPE context
 * @param ls Specifies the starting local store destination address.
 * @param ea Specifies the starting effective address source address.
 * @param size Specifies the size, in bytes, to be transferred.
 * @param tag Specifies the tag id used to identify the DMA command.
 * @param tid Specifies the transfer class identifier of the DMA command.
 * @param rid Specifies the replacement class identifier of the DMA command.
 * @return On success, return 0. On failure, -1 is returned.
 */
int _base_spe_mfcio_getf(spe_context_ptr_t spectx, 
			unsigned int ls, 
			void *ea, 
			unsigned int size, 
			unsigned int tag, 
			unsigned int tid, 
			unsigned int rid);
                       
/**
 *        The _base_spe_out_mbox_read function reads the contents of the SPE outbound interrupting
 *        mailbox for the SPE thread speid.
 * 
 *        The call will not block until the read request is satisfied,
 *        but instead return up to count currently available mailbox entries.
 * 
 *        spe_stat_out_intr_mbox can be called to ensure that data is available prior 
 *        to reading the outbound interrupting mailbox.
 * 
 * @param spectx  Specifies the SPE thread whose outbound mailbox is to be read.
 * @param mbox_data
 * @param count
 * 
 *  
 * @retval       >0       the number of 32-bit mailbox messages read
 * @retval       =0       no data available
 * @retval       -1       error condition and errno is set\n
 * 		Possible values for errno:\n
 * 		EINVAL     speid is invalid\n
 * 		Exxxx      what else do we need here??
 */
int _base_spe_out_mbox_read(spe_context_ptr_t spectx, 
			unsigned int mbox_data[], 
			int count);

/**
 *        The _base_spe_in_mbox_write function writes mbox_data to the SPE inbound
 *        mailbox for the SPE thread speid.
 * 
 *        If the behavior flag indicates ALL_BLOCKING the call will try to write exactly count mailbox entries
 *        and block until the write request is satisfied, i.e., exactly count mailbox entries
 *        have been written.
 *        If the behavior flag indicates ANY_BLOCKING the call will try to write up to count mailbox entries,
 *        and block until the write request is satisfied, i.e., at least 1 mailbox entry has been written.
 *        If the behavior flag indicates ANY_NON_BLOCKING the call will not block until the write request is satisfied,
 *        but instead write whatever is immediately possible and return the number of mailbox entries written.
 *        spe_stat_in_mbox can be called to ensure that data can be written prior 
 *        to calling the function.
 * 
 * @param spectx Specifies the SPE thread whose outbound mailbox is to be read.
 * @param mbox_data
 * @param count
 * @param behavior_flag 
 *           ALL_BLOCKING\n
 *           ANY_BLOCKING\n
 *           ANY_NON_BLOCKING\n
 *  
 * @retval       >=0      the number of 32-bit mailbox messages written
 * @retval       -1       error condition and errno is set\n
 * Possible values for errno:\n
 * 	EINVAL     spectx is invalid\n
 * 	Exxxx      what else do we need here??
 */
int _base_spe_in_mbox_write(spe_context_ptr_t spectx, 
			unsigned int mbox_data[], 
			int count, 
			int behavior_flag);

/**
 * The _base_spe_in_mbox_status function fetches the status of the SPU inbound mailbox for 
 * the SPE thread specified by the speid parameter. A 0 value is return if the 
 * mailbox is full. A non-zero value specifies the number of available (32-bit) 
 * mailbox entries.
 * 
 * @param spectx Specifies the SPE context whose mailbox status is to be read.
 * @return On success, returns the current status of the mailbox, respectively. 
 * On failure, -1 is returned.
 * @sa spe_read_out_mbox, spe_write_in_mbox, read (2)
 */
int _base_spe_in_mbox_status(spe_context_ptr_t spectx);

/**
 * The _base_spe_out_mbox_status function fetches the status of the SPU outbound mailbox 
 * for the SPE thread specified by the speid parameter. A 0 value is return if 
 * the mailbox is empty. A non-zero value specifies the number of 32-bit unread 
 * mailbox entries.
 * 
 * @param spectx Specifies the SPE context whose mailbox status is to be read.
 * @return On success, returns the current status of the mailbox, respectively. 
 * On failure, -1 is returned.
 * @sa spe_read_out_mbox, spe_write_in_mbox, read (2)
 */
int _base_spe_out_mbox_status(spe_context_ptr_t spectx);

/**
 * The _base_spe_out_intr_mbox_status function fetches the status of the SPU outbound 
 * interrupt mailbox for the SPE thread specified by the speid parameter. A 0 
 * value is return if the mailbox is empty. A non-zero value specifies the number 
 * of 32-bit unread mailbox entries.
 * 
 * @param spectx Specifies the SPE context whose mailbox status is to be read.
 * @return On success, returns the current status of the mailbox, respectively. 
 * On failure, -1 is returned.
 * @sa spe_read_out_mbox, spe_write_in_mbox, read (2)
 */
int _base_spe_out_intr_mbox_status(spe_context_ptr_t spectx);

/**
 * The _base_spe_out_intr_mbox_read function reads the contents of the SPE outbound interrupting
 * mailbox for the SPE context.
 */
int _base_spe_out_intr_mbox_read(spe_context_ptr_t spectx, 
			unsigned int mbox_data[], 
			int count, 
			int behavior_flag);

/**
 * The _base_spe_signal_write function writes data to the signal notification register 
 * specified by signal_reg for the SPE thread specified by the speid parameter.
 * 
 * @param spectx Specifies the SPE context whose signal register is to be written to.
 * @param signal_reg Specified the signal notification register to be written. 
 * Valid signal notification registers are:\n
 * SPE_SIG_NOTIFY_REG_1 SPE signal notification register 1\n
 * SPE_SIG_NOTIFY_REG_2 SPE signal notification register 2\n
 * @param data The 32-bit data to be written to the specified signal notification 
 * register.
 * @return On success, spe_write_signal returns 0. On failure, -1 is returned.
 * @sa spe_get_ps_area, spe_write_in_mbox
 */ 
int _base_spe_signal_write(spe_context_ptr_t spectx, 
                        unsigned int signal_reg, 
                        unsigned int data );

/**
 * register a handler function for the specified number
 * NOTE: registering a handler to call zero and one is ignored.
 */
extern int _base_spe_callback_handler_register(void * handler, unsigned int callnum, unsigned int mode);

/**
 * unregister a handler function for the specified number
 * NOTE: unregistering a handler from call zero and one is ignored.
 */
extern int _base_spe_callback_handler_deregister(unsigned int callnum );

/**
 * query a handler function for the specified number
 */
extern void * _base_spe_callback_handler_query(unsigned int callnum );

/**
 * _base_spe_stop_reason_get
 * 
 * @param       spectx       one thread for which to check why it was stopped
 * 
 * @retval 0        success - eventid and eventdata set appropriately
 * @retval 1         spe has not stopped after checking last, so no data was written
 *                  to event
 * @retval      -1         an error has happened, event was not touched, errno gets set\n
 * Possible vales for errno:\n
 * EINVAL     speid is invalid\n
 * Exxxx      what else do we need here??
 */
int _base_spe_stop_reason_get(spe_context_ptr_t spectx);
			

/**
 * _base_spe_mfcio_tag_status_read 
 * 
 * No Idea 
 * */
int _base_spe_mfcio_tag_status_read(spe_context_ptr_t spectx, unsigned int mask, unsigned int behavior, unsigned int *tag_status);

/**
 * __base_spe_stop_event_source_get
 * 
 * @param spectx Specifies the SPE context
 */
int __base_spe_stop_event_source_get(spe_context_ptr_t spectx);

/**
 * __base_spe_stop_event_target_get
 * 
 * @param spectx Specifies the SPE context
 */
int __base_spe_stop_event_target_get(spe_context_ptr_t spectx);

/**
 * _base_spe_stop_status_get
 * 
 * @param spectx Specifies the SPE context
 * 
 */
int _base_spe_stop_status_get(spe_context_ptr_t spectx);

/**
 * __base_spe_event_source_acquire opens a file descriptor to the specified event source 
 * 
 * @param spectx Specifies the SPE context
 * 
 * @param fdesc Specifies the event source
 */
int __base_spe_event_source_acquire(struct spe_context *spectx, enum fd_name fdesc);

/**
 * __base_spe_event_source_release releases the file descriptor to the specified event source
 * 
 * @param spectx Specifies the SPE context
 * 
 * @param fdesc Specifies the event source
  */
void __base_spe_event_source_release(struct spe_context *spectx, enum fd_name fdesc);

/**
 * _base_spe_ps_area_get returns a pointer to the start of memory mapped problem state
 * area 
 * 
 * @param spectx Specifies the SPE context
 * 
 * @param area specifes the area to map
 */
void* _base_spe_ps_area_get(struct spe_context *spectx, enum ps_area area);

/**
 * __base_spe_spe_dir_get return the file descriptor of the SPE directory in spufs
 * 
 * @param spectx Specifies the SPE context
 */
int __base_spe_spe_dir_get(struct spe_context *spectx);

/**
 * _base_spe_ls_area_get returns a pointer to the start of the memory mapped
 * local store area
 * 
 * @param spectx Specifies the SPE context
 */
void* _base_spe_ls_area_get(struct spe_context *spectx);

/**
 * _base_spe_ls_size_get returns the size of the local store area
 * 
 * @param spectx Specifies the SPE context
 */
int _base_spe_ls_size_get(spe_context_ptr_t spe);

/**
 * _base_spe_context_lock locks members of the SPE context
 *
 * @param spectx Specifies the SPE context 
 * @param fd Specifies the file
 */
void _base_spe_context_lock(spe_context_ptr_t spe, enum fd_name fd);

/**
 * _base_spe_context_unlock unlocks members of the SPE context
 *
 * @param spectx Specifies the SPE context
 * @param fd Specifies the file
 */
void _base_spe_context_unlock(spe_context_ptr_t spe, enum fd_name fd);

/**
 * _base_spe_info_get
 */
int _base_spe_cpu_info_get(int info_requested, int cpu_node);

/**
 * __spe_context_update_event internal function for gdb notification.
 * 
 */
void  __spe_context_update_event(void);

/**
 * _base_spe_mssync_start starts Multisource Synchronisation
 *
 * @param spectx Specifies the SPE context
 */
int _base_spe_mssync_start(spe_context_ptr_t spectx);

/**
 * _base_spe_mssync_status retrieves status of Multisource Synchronisation
 *
 * @param spectx Specifies the SPE context
 */
int _base_spe_mssync_status(spe_context_ptr_t spectx);


#ifdef __cplusplus
}
#endif

#endif
