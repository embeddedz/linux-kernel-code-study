start_kernel	  
{
	lock_kernel								// aquire the big kernel sem
	
	tick_init	
	{
		clockevents_register_notifier	//register notifier
	}
		
	boot_cpu_init					//init cpu maps
	page_address_init			//init high memory page hash table
	
	printk(linux_banner)
	
	setup_arch
	{
	
		setup_processor();							//set up information from proc_info_list
	
		mdesc = setup_machine(machine_arch_type);	//get machine descriptor
	
		if (tags->hdr.tag == ATAG_CORE) {				/*parse tags:store command line in   
				if (meminfo.nr_banks != 0)			  default_command_line; mem in
					squash_mem_tags(tags);          meminfo*/
				parse_tags(tags);
			}
	
		parse_cmdline(cmdline_p, from);		//parse some early_params of command line
		/******************************************************************************************/
		/*MILESTONE1:__create_page_tables called in stext created a coarse page table, paging_init will establish a fully fledged one*/
		/******************************************************************************************/
		paging_init(&meminfo, mdesc);	//set up page tables    
		{
			build_mem_type_table();	//set up mem_types, purpose unknown
			prepare_page_table(mi); 	//clear all pmds
			
			/*this is for boot mem allocator*/
			bootmem_init(mi);			//set up page table mappings for every node   
			{
				end_pfn = bootmem_init_node(node, initrd_node, mi);	/*set up mapping for banks within the node*/     
				{
					map_memory_bank(bank);	//mapping for one bank, set up pmds, ptes;
					init_bootmem_node		//set up the page bitmap for this node
					free_area_init_node		//mark node free for use, init zones                  
				}
	    }      
	    /*this is for boot mem allocator*/
	    
			devicemaps_init			//set up mapping for device    
			{	
				/*
	 			* Create a mapping for the machine vectors at the high-vectors
	 			* location (0xffff0000).  If we aren't using high-vectors, also
	 			* create a mapping at the low-vectors virtual address.
	 			*/
				map.pfn = __phys_to_pfn(virt_to_phys(vectors));
				map.virtual = 0xffff0000;
				map.length = PAGE_SIZE;
				map.type = MT_HIGH_VECTORS;
				create_mapping(&map);
	
				/****************************************************************/
				/*this machine dependent function may do a lot more than it seems*/
				/****************************************************************/
				mdesc->map_io();		//call machine descriptor's map_io, s3c24xx map io to virtual memory starting from 0xF0000000      
														//in the case of smdk2410_map_io, it calls s3c244x_init_clocks(which calls a series of s3c24xx_register_clock)
														// for "time_init" to use later, and s3c244x_init_uarts
			}
		}
		cpu_init		//print current cpu info, init exception stack
	}	
	
	setup_command_line	//just save command line to some global variables
	
	setup_per_cpu_areas		//use alloc_bootmem to allocate memory for per_cpu data statical 
												//defined in program and copy these data from compiler assigned 
												//address to allocator specified areas
	
	smp_prepare_boot_cpu	//per_cpu(cpu_data, cpu).idle = current, set current process as the idle process
	
	sched_init           //initialize per_cpu rq, priority queue(within rq)
	{	
		/* idle thread was statically initialized using init_thread_union	and put in .data.init_task section. after setting up its sp in __mmap_switched, we can refer to it through "current"*/
		init_idle					 //init fields inside of task_struct of idle(which is equvalent to "init_task")
	}
	
	build_all_zonelists		    
	page_alloc_init			//if defined cpu_hotplug, register notifier to free pages when cpu off
	
	parse_early_param		//handle early params, handler registered using "early_param(str, fn)"
	
	parse_args("Booting kernel", static_command_line, __start___param,		//parse and handle "module parameters"
		   __stop___param - __start___param,
		   &unknown_bootoption);
	
	sort_main_extable		//something related to page_fault_handle
	
	trap_init						//before enable interrupt, vecter table needs to be relocated to high vecter page(0xffff0000), 
											//vecter tables including __vectors_start~__vectors_end, __stubs_start~__stubs_end, __kuser_helper_start~__kuser_helper_start+kuser_sz, sigreturn_codes
	
	rcu_init						//register_cpu_notifier for rcu_cpu_notify which initializes per_cpu rcu lists and the tasklet to handle rcu callbacks
	
	init_IRQ						
	{
		init_arch_irq			//point to mdesc->init_irq, initialized previously in "setup_arch"; each irq correspond to a "irq_desc", set it up by calling set_irq_chip and set_irq_handler
	}
	
	pidhash_init				//init pid hash table
	
	init_timers					//register_cpu_notifier for timer_cpu_notify, open_softirq to handle dynamic timer requested
											//timer_cpu_notify calls init_timers_cpu, but since slab allocator is not already, just set some flags and get out
											
	hrtimers_init				//similar to above
	
	softirq_init				//open_softirq for tasklet_action&tasklet_hi_action				
	
	timekeeping_init		//init clocksource and xtime
	
	time_init						//call system_timer->init(), system_timer = mdesc->timer was set in setup_arch.
											//in the case of smdk-24xx, it end up calling s3c2410_timer_setup=>calls s3c2410_clkcon_enable
											//then calls setup_irq(IRQ_TIMER4, &s3c2410_timer_irq). the timer interrupt will call timer_tick which updates jiffies
	
	/******************************************************************************************/
	/*MILESTONE2:enable interrupt, timer is running*/
	/******************************************************************************************/										
	local_irq_enable
											
	console_init				
	{
		tty_register_ldisc(N_TTY, &tty_ldisc_N_TTY)	
		call = __con_initcall_start;					//calls all initcall from section "con_initcall.init"
		while (call < __con_initcall_end) 		//in the case of s3c24xx, there's only one, s3c24xx_serial_initconsole
		{																			//which calls s3c24xx_serial_init_ports(set up uart_port) and register_console()
			(*call)();										
			call++;
		}
	}	
	/*from now on, printk can output message on console*/
	
	vfs_caches_init_early		//allocate dentry and inode hashtable, using bootmem
	
	cpuset_init_early				//cpuset is used to assigning a set of CPUs and Memory Nodes to a set of tasks, google it if want to know more
	mem_init								// marks the free areas in the mem_map and tells us how much memory is free
	
	kmem_cache_init					//initilize slab allocator which is the memory allocator after boot phase.
	/******************************************************************************************/
	/*MILESTONE3:slab allocator is running, kmalloc, kmem_cache_alloc is available*/
	/******************************************************************************************/
	
	setup_per_cpu_pageset		
	
	pidmap_init							//create pid slab cache
	
	pgtable_cache_init			//empty for most arch
	
	fork_init
	{
		kmem_cache_create("task_struct"					//create task_struct cache
		init_task.signal->rlim[RLIMIT_NPROC].rlim_cur = max_threads/2;	//set init task resource limit
		...
	}
	
	proc_caches_init						//create lots of slab cache like:mm_struct,vm_area_struct,files_struct, etc
	
	buffer_init							//create buffer head cache, and register cpu notifier
	
	unnamed_dev_init				//calls idr_init,Idr is a set of library functions for the management of small integer ID numbers
	
	key_init								//initialise the key management stuff
	
	security_init						//call security_initcall in section
	
	vfs_caches_init					//create slab cache and hash table for vfs data(dentry, inode, file,vfsmount)
	{
		dcache_init(mempages);
		inode_init(mempages);
		files_init(mempages);
		mnt_init(mempages);
		
		bdev_cache_init();			//create slab cache for "bdev_inode", register_filesystem(&bd_type), mount bdev filesystem
		chrdev_init();					//init cdev_map, set probe function to base_probe(calls request_module => modprobe)
	}
	
	page_writeback_init				//something about dirty page write back
	
	proc_root_init						
	{
		register_filesystem(&proc_fs_type)			//register proc filesystem to the vfs
		
		kern_mount(&proc_fs_type)								//mount procfs
			
		proc_misc_init
		{
			.....										//create many proc entries and their operation function
			
			entry = create_proc_entry("kmsg", S_IRUSR, &proc_root);		//create kmsg entry where we can access printk messages 
			entry->proc_fops = &proc_kmsg_operations;
			
			create_seq_entry("devices", 0, &proc_devinfo_operations);	//create procfs entries for others like devices, cpuinfo, etc
			create_seq_entry("cpuinfo", 0, &proc_cpuinfo_operations);
			......	
		}
		
		.....
		proc_net = proc_mkdir("net", NULL);
		.....
		proc_root_driver = proc_mkdir("driver", NULL);
		proc_mkdir("fs/nfsd", NULL);
		.....
		proc_tty_init		
		.....
		proc_bus = proc_mkdir("bus", NULL);
		proc_sys_init
	}
	
	cpuset_init				//register_filesystem(&cpuset_fs_type), cpuset is used to assigning a set of CPUs and Memory Nodes to a set of tasks, google it if want to know more
	
	taskstats_init_early();		//Taskstats is a netlink-based interface for sending per-task and per-process statistics from the kernel to userspace
	delayacct_init();					//Tasks encounter delays in execution when they wait for some kernel resource to become available, The per-task delay accounting functionality measures the delays experienced by a task
	
	acpi_early_init		//ACPI modules are kernel modules for different ACPI parts. They enable special ACPI functions or add information to /proc or /sys. These information can be parsed by acpid for events or other monitoring applications.
	
	rest_init
	{
		kernel_thread(kernel_init, NULL, CLONE_FS | CLONE_SIGHAND);				//create kernel_init thread, pid will be 1, 0 is the boot/idle thread
		//review kernel_thread related knowledge
		
		pid = kernel_thread(kthreadd, NULL, CLONE_FS | CLONE_FILES);			//create kthreadd thread, pid will be 2, it will loop and respond to kthread_create to 
																																			//spawn new kernel thread to run functions
																																			
		schedule()						//start schedule, kernel_init&kthreadd will be picked by scheduler and start to run
		
		cpu_idle							//root thread goes into idle thread
		{
			.....
			while (!need_resched())
			idle();										//idle thread will call arch dependent idle function, normally it enters a sleep state waiting for interrupt
			
			.....
		}	
		
	}
}

/******************************************************************************************/
/*MILESTONE4:process 0 finished its mission and goes into idle thread, now process 1 takes charge
/******************************************************************************************/
kernel_init
{
	smp_prepare_cpus			//boot other cores to enter WFI mode
	
	do_pre_smp_initcalls	
	{
		migration_init			//register register_cpu_notifier, once a core is up, he spawns his migration&ksoftirq&softlockup thread
		spawn_ksoftirqd
		spawn_softlockup_task
	}
	
	smp_init	
	{
		for_each_present_cpu(cpu) 
			if (!cpu_online(cpu))
				cpu_up(cpu);					
					_cpu_up
					{
						__cpu_up
						{
							fork_idle(cpu)			//copy a idle thread for each cpu
							
							pgd = pgd_alloc(&init_mm);				//init page table to allow the new CPU to	enable the MMU safely
							pmd = pmd_offset(pgd, PHYS_OFFSET);
							*pmd = __pmd((PHYS_OFFSET & PGDIR_MASK) |
		     						PMD_TYPE_SECT | PMD_SECT_AP_WRITE);
		     						
		     			secondary_data.stack = task_stack_page(idle) + THREAD_START_SP;					//tell the secondary core where to find																																											
							secondary_data.pgdir = virt_to_phys(pgd);																//its stack and the page tables.
							wmb();
							
							boot_secondary(cpu, idle)					
							{
								smp_cross_call(cpumask_of_cpu(cpu))				//wake cpu up from WFI by using cross smp interrupt				
								while (time_before(jiffies, timeout)) 
								{
									smp_rmb();
									if (pen_release == -1)									//wait for secondary cores to change the flag
											break;
										udelay(10);
								}										
								/******************************************************************************************/
								/*MILESTONE5:all cpus will wake up from WFI and start executing "secondary_startup"
								/******************************************************************************************/		
								
							}
						}
						raw_notifier_call_chain					//call all registered cpu notifier
					}
		smp_cpus_done				//output smp info, all secondary cpu has finished booting
	}			
	
	sched_init_smp()
	{
		arch_init_sched_domains(&cpu_online_map)				
		
		hotcpu_notifier(update_sched_domains, 0);		//register cpu notifier to update sched domain if cpu hotplugable						
	}
	
	cpuset_init_smp								//cpuset is used to assigning a set of CPUs and Memory Nodes to a set of tasks, google it if want to know more
	
	do_basic_setup
	{
		init_workqueues();
		{
			keventd_wq = create_workqueue("events")
			{
				for_each_possible_cpu(cpu) 				//create worker thread for each cpu
				{
					cwq = init_cpu_workqueue(wq, cpu);
					if (err || !cpu_online(cpu))
					continue;
					err = create_workqueue_thread(cwq, cpu);
					start_workqueue_thread(cwq, cpu);
				}
		}		
		
		usermodehelper_init();
		{
			khelper_wq = create_singlethread_workqueue("khelper");
			{
				for_each_possible_cpu(cpu)           //create worker thread for each cpu       
				{
					cwq = init_cpu_workqueue(wq, cpu);
					if (err || !cpu_online(cpu))
					continue;
					err = create_workqueue_thread(cwq, cpu);
					start_workqueue_thread(cwq, cpu);
					
				}
			}
		}	
		
		driver_init()		//initialize driver model
		{
			devices_init();			//register device subsystem
			buses_init();				//register bus subsystem
			classes_init();			//register and init class subsystem
			firmware_init();		//register firmware subsystem
			hypervisor_init();	//register hypervisor subsystem
  		
			/* These are also core pieces, but must come after the
			 * core core pieces.
			 */
			platform_bus_init();
			{
				device_register(&platform_bus)
				bus_register(&platform_bus_type)
			}				
			system_bus_init();	//register system_subsys			
			cpu_dev_init();			//sysdev_class_register(&cpu_sysdev_class)
			memory_dev_init();	
			{
				sysdev_class_register(&memory_sysdev_class)
				for (i = 0; i < NR_MEM_SECTIONS; i++) 
				{
					err = add_memory_block(0, __nr_to_section(i), MEM_ONLINE, 0);		//create sys dev for memory, will appear under /sys/devices/
				}
				memory_probe_init		//create sys class attributes
				block_size_init			//create sys class attributes
			}
			
			attribute_container_init();		//INIT_LIST_HEAD(&attribute_container_list);
		}
		init_irq_proc();				//create /proc/irq and create entries for all NR_IRQS
		do_initcalls();
		{
			/*pure_initcall*/		
			/*core_initcall*/
			/*postcore_initcall*/
			/*arch_initcall*/
			/*subsys_initcall*/
			/*fs_initcall*/
			/*rootfs_initcall*/
			/*device_initcall*/
			/*late_initcall*/
			/*__initcall*/
		}
	}
	
	prepare_namespace
	{
		if (saved_root_name[0]) 			//saved_root_name is the name that passed as tags "root=" to the OS
		{
			root_device_name = saved_root_name;
			if (!strncmp(root_device_name, "mtd", 3)) 
			{
				mount_block_root(root_device_name, root_mountflags);		//if root filesystem in on mtd device
				goto out;
			}
			ROOT_DEV = name_to_dev_t(root_device_name);
			if (strncmp(root_device_name, "/dev/", 5) == 0)
				root_device_name += 5;
		}
		
		mount_root							//if root filesystem is not on mtd device
		{
			#ifdef CONFIG_ROOT_NFS
				if (MAJOR(ROOT_DEV) == UNNAMED_MAJOR) 
					mount_nfs_root()
					{
						do_mount_root("/dev/root", "nfs", root_mountflags, data) == 0)
					}	
			#ifdef CONFIG_BLOCK
				create_dev("/dev/root", ROOT_DEV);
				mount_block_root("/dev/root", root_mountflags);
				{
					get_fs_names(fs_names)
					do_mount_root(name, p, flags, root_mount_data)
					{
						sys_mount(name, "/root", fs, flags, data)			//mount root filesystem
						sys_chdir("/root")				
					}
				}
		}
	}
/******************************************************************************************/
/*MILESTONE7:root filesystem mounted, we can now access program using their path
/******************************************************************************************/		
	init_post
	{
		free_initmem						//free memory from __init_begin to __init_end
		
		sys_open((const char __user *) "/dev/console", O_RDWR, 0)			//open console, /dev/console is manually created in root filesystem, 
																																	//that's why we can access it before udev started
		(void) sys_dup(0);				//fd 0,1,2 all point to /dev/console
		(void) sys_dup(0);																				
		
		.....
		//run init process enter user space											
	}
}






/******************************************************************************************/
/*MILESTONE6:other cores will call "secondary_start_kernel" after some processor setup in "secondary_startup"
/******************************************************************************************/		
secondary_start_kernel
{
	cpu_init()					//init exception stack, same with "cpu_init" in boot cpu 
	
	platform_secondary_init(cpu)
	{
		pen_release = -1;		//so main core knows this cpu is up
		smp_wmb();
	}	
		
	/* Enable local interrupts*/
	local_irq_enable();
	local_fiq_enable();
	
	smp_store_cpu_info(cpu);
	
	cpu_set(cpu, cpu_online_map);		//this cpu officially online
	
	local_timer_setup(cpu);
	
	cpu_idle();										//go into the per cpu idle thread
}
	