# -*-Makefile-*-

OBJDIRS		+= $(KERN_OBJDIR)/dev

# Use a single wildcard to include all existing .c and .S files in kern/dev/ and subdirectories
KERN_SRCFILES	+= $(wildcard $(KERN_DIR)/dev/*.c $(KERN_DIR)/dev/disk/*.c $(KERN_DIR)/dev/*.S)

# Ensure object files are placed in the correct directory
# Remove the extra 'kern' in the path
$(KERN_OBJDIR)/dev/%.o: $(KERN_DIR)/dev/%.c
	@echo + cc[KERN/dev] $<
	@mkdir -p $(@D)
	$(V)$(CC) $(KERN_CFLAGS) -c -o $@ $<

$(KERN_OBJDIR)/dev/%.o: $(KERN_DIR)/dev/%.S
	@echo + as[KERN/dev] $<
	@mkdir -p $(@D)
	$(V)$(CC) $(KERN_CFLAGS) -c -o $@ $<

$(KERN_OBJDIR)/dev/disk/%.o: $(KERN_DIR)/dev/disk/%.c
	@echo + cc[KERN/dev/disk] $<
	@mkdir -p $(@D)
	$(V)$(CC) $(KERN_CFLAGS) -c -o $@ $<
