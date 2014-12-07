
/*  
 *  hello-2.c - Demonstrating the module_init() and module_exit() macros.
 *  This is preferred over using init_module() and cleanup_module().
 */
#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/init.h>		/* Needed for the macros */
#include <linux/mm_types.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <asm/pgtable.h>
#include <linux/mm.h>
#include <linux/bootmem.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/gfp.h>

#define N_FILES_MAX 500
static struct proc_dir_entry *proc_rpc;

static char *file_names[N_FILES_MAX] = {0};
static int pages_per_file[N_FILES_MAX] = {0};
void print_results(void);


int account_file_page(char *name_to_add)
{
	int i = 0;
	for (; i < N_FILES_MAX; i++) {
		if (!file_names[i]) {
			file_names[i] = kzalloc(strlen(name_to_add) + 1, GFP_KERNEL);
			if (!file_names[i])
				return -1;
			memcpy(file_names[i], name_to_add, strlen(name_to_add));
			pages_per_file[i]++;
			return 0;
		}
		if (!strcmp(file_names[i], name_to_add)) {
			pages_per_file[i]++;
			return 0;
		}
	}
	printk(KERN_ERR "Please, increase N_FILES_MAX, the limit is reached\n");
	return 0;
}

void print_results()
{
        int i = 0;
	unsigned long n_pages = 0;
        for (; i < N_FILES_MAX; i++) {
		if (file_names[i]) {
			printk(KERN_ERR "%-3d %-30s %dK\n", i, file_names[i], pages_per_file[i] * 4);
			n_pages += pages_per_file[i];
			kfree(file_names[i]);
			file_names[i] = NULL;
			pages_per_file[i] = 0;
		} else
			break;
	}
	printk(KERN_ERR "Overall: %luK\n", n_pages * 4);
}

int do_pfn_stuff(unsigned long pfn)
{
	int ret = 0;
	struct page *page;
	page = pfn_to_page(pfn);
	if (!PageAnon(page)) {
		struct inode *inode;
		struct address_space *map = page->mapping;
		struct dentry *dentry;
		if (!map) 
                	return -1;
		inode = map->host;
                if (!inode)
 			return -1;
		spin_lock(&inode->i_lock);
	        if (hlist_empty(&inode->i_dentry)) {
			spin_unlock(&inode->i_lock);
			return ret;
		}
		dentry = hlist_entry(inode->i_dentry.first, struct dentry, d_alias);
		spin_lock(&dentry->d_lock);
		ret |= account_file_page(dentry->d_name.name);
		spin_unlock(&dentry->d_lock);
		spin_unlock(&inode->i_lock);
	}
	return ret;
}

static int rpc_proc_show(struct seq_file *m, void *v) 
{
	int ret = 0;
        unsigned long pfn = min_low_pfn;
	printk(KERN_ERR "min_low_pfn: %lx max_pfn: %lx\n", min_low_pfn, max_pfn);
        do {
                if (pfn_valid(pfn))
                        ret = do_pfn_stuff(pfn);
                pfn++;
        } while (pfn < max_pfn);
	print_results();
        return 0;
}

static int rpc_proc_open(struct inode *inode, struct  file *file) {
	return single_open(file, rpc_proc_show, NULL);
}

static const struct file_operations rpc_proc_fops = {
	.read = rpc_proc_show,
};

static int __init readpcache(void)
{
	proc_create("read_pcache", 0, NULL, &rpc_proc_fops);
	return 0;
}

static void __exit unload(void)
{
}

module_init(readpcache);
module_exit(unload);
