/**
 * @file   time_synchronizer_lkm.c
 * @author Tao Wang
 * @date   15 March 2018
 * @brief  A kernel module of time synchronizer using external interrupt input that is connected to
 * a GPIO. It has full support for interrupts and for sysfs entries so that an interface
 * can be created to the external interrupt.
 * The sysfs entry appears at /sys/ts_lkm/gpio388
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>       // Required for the GPIO functions
#include <linux/interrupt.h>  // Required for the IRQ code
#include <linux/kobject.h>    // Using kobjects for the sysfs bindings
#include <linux/time.h>       // Using the clock to measure time between interrupts
#define  DEBOUNCE_TIME 200    ///< The default bounce time -- 200ms

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tao Wang");
MODULE_DESCRIPTION("A Time Synchronizer LKM using GPIO interrupt");
MODULE_VERSION("0.1");

static bool isRising = 1;                   ///< Rising edge is the default IRQ property
module_param(isRising, bool, S_IRUGO);      ///< Param desc. S_IRUGO can be read/not changed
MODULE_PARM_DESC(isRising, "Rising edge = 1 (default), falling edge = 0");  ///< parameter description

static unsigned int gpioTS = 422;       ///< Default GPIO is 388
module_param(gpioTS, uint, S_IRUGO);    ///< Param desc. S_IRUGO can be read/not changed
MODULE_PARM_DESC(gpioTS, "Time Synchronizer GPIO number (default=388)");  ///< parameter description

static char   gpioName[8] = "gpioXXX";      ///< Null terminated default string -- just in case
static int    irqNumber;                    ///< Used to share the IRQ number within this file
static int    numberInterrupts = 0;            ///< For information, store the number of interrupts
static bool   newInterrupt = 0;				///< Flag for new interrupt, set in irq handler, clear by application when reading interrupt time.
static bool   isDebounce = 0;               ///< Use to store the debounce state (off by default)
static struct timespec timeInterrupt; ///< timespecs from linux/time.h (has nano precision)

/// Function prototype for the custom IRQ handler function -- see below for the implementation
static irq_handler_t  tsgpio_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs);

/** @brief A callback function to output the numberInterrupts variable
 *  @param kobj represents a kernel object device that appears in the sysfs filesystem
 *  @param attr the pointer to the kobj_attribute struct
 *  @param buf the buffer to which to write the number of interrupts
 *  @return return the total number of characters written to the buffer (excluding null)
 */
static ssize_t numberInterrupts_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
   return sprintf(buf, "%d\n", numberInterrupts);
}

/** @brief A callback function to read in the numberInterrupts variable
 *  @param kobj represents a kernel object device that appears in the sysfs filesystem
 *  @param attr the pointer to the kobj_attribute struct
 *  @param buf the buffer from which to read the number of interrupts (e.g., reset to 0).
 *  @param count the number characters in the buffer
 *  @return return should return the total number of characters used from the buffer
 */
static ssize_t numberInterrupts_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
   sscanf(buf, "%du", &numberInterrupts);
   return count;
}

/** @brief A callback function to output the timeInterrupt variable
 *  @param kobj represents a kernel object device that appears in the sysfs filesystem
 *  @param attr the pointer to the kobj_attribute struct
 *  @param buf the buffer to which to write the interrupt time
 *  @return return the total number of characters written to the buffer (excluding null)
 */
static ssize_t timeInterrupt_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
   return sprintf(buf, "%lu.%.9lu \n", timeInterrupt.tv_sec, timeInterrupt.tv_nsec );
}

/** @brief A callback function to read in the timeInterrupt variable
 *  @param kobj represents a kernel object device that appears in the sysfs filesystem
 *  @param attr the pointer to the kobj_attribute struct
 *  @param buf the buffer from which to read the interrupt time (e.g., reset to 0).
 *  @param count the number characters in the buffer
 *  @return return should return the total number of characters used from the buffer
 */
static ssize_t timeInterrupt_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	long int time_interrupt;
	sscanf(buf, "%lu", &time_interrupt);
	timeInterrupt.tv_sec = time_interrupt / (10^9);
	timeInterrupt.tv_nsec = time_interrupt % (10^9);
	return count;
}

/** @brief Displays if there is new interrupt */
static ssize_t newInterrupt_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
   return sprintf(buf, "%d\n", newInterrupt);
}

/** @brief Stores and sets the newInterrupt state */
static ssize_t newInterrupt_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
   unsigned int temp;
   sscanf(buf, "%du", &temp);                // use a temp varable for correct int->bool
   newInterrupt = temp;
   return count;
}

/** @brief Displays if time synchronize debouncing is on or off */
static ssize_t isDebounce_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
   return sprintf(buf, "%d\n", isDebounce);
}

/** @brief Stores and sets the debounce state */
static ssize_t isDebounce_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
   unsigned int temp;
   sscanf(buf, "%du", &temp);                // use a temp varable for correct int->bool
   gpio_set_debounce(gpioTS,0);
   isDebounce = temp;
   if(isDebounce) { gpio_set_debounce(gpioTS, DEBOUNCE_TIME);
      printk(KERN_INFO "Time Synchronizer: Debounce on\n");
   }
   else { gpio_set_debounce(gpioTS, 0);  // set the debounce time to 0
      printk(KERN_INFO "Time Synchronizer: Debounce off\n");
   }
   return count;
}

/**  Use these helper macros to define the name and access levels of the kobj_attributes
 *  The kobj_attribute has an attribute attr (name and mode), show and store function pointers
 *  The count variable is associated with the numberInterrupts variable and it is to be exposed
 *  with mode 0666(0664 for new kernel) using the numberInterrupts_show and numberInterrupts_store functions above
 */
static struct kobj_attribute count_attr = __ATTR(numberInterrupts, 0664, numberInterrupts_show, numberInterrupts_store);
static struct kobj_attribute time_int_attr = __ATTR(timeInterrupt, 0664, timeInterrupt_show, timeInterrupt_store);
static struct kobj_attribute debounce_attr = __ATTR(isDebounce, 0664, isDebounce_show, isDebounce_store);
static struct kobj_attribute new_int_attr = __ATTR(newInterrupt, 0664, newInterrupt_show, newInterrupt_store);

/**  The __ATTR_RO macro defines a read-only attribute. There is no need to identify that the
 *  function is called _show, but it must be present. __ATTR_WO can be  used for a write-only
 *  attribute but only in Linux 3.11.x on.
 */

/**  The ts_attrs[] is an array of attributes that is used to create the attribute group below.
 *  The attr property of the kobj_attribute is used to extract the attribute struct
 */
static struct attribute *ts_attrs[] = 
{
      &count_attr.attr,                  ///< The number of interrupts
      &time_int_attr.attr,						 ///< The time interrupt happens
      &debounce_attr.attr,               ///< Is the debounce state true or false
      &new_int_attr.attr,				 ///< Is there new interrupt
      NULL,
};

/**  The attribute group uses the attribute array and a name, which is exposed on sysfs -- in this
 *  case it is gpio115, which is automatically defined in the ts_init() function below
 *  using the custom kernel parameter that can be passed when the module is loaded.
 */
static struct attribute_group attr_group = 
{
      .name  = gpioName,                 ///< The name is generated in ts_init()
      .attrs = ts_attrs,                ///< The attributes array defined just above
};

static struct kobject *ts_kobj;

/** @brief The LKM initialization function
 *  The static keyword restricts the visibility of the function to within this C file. The __init
 *  macro means that for a built-in driver (not a LKM) the function is only used at initialization
 *  time and that it can be discarded and its memory freed up after that point. In this example this
 *  function sets up the GPIOs and the IRQ
 *  @return returns 0 if successful
 */
static int __init ts_init(void){
   int result = 0;
   int IRQflags = IRQF_TRIGGER_RISING;      // The default is a rising-edge interrupt

   printk(KERN_INFO "Time Synchronizer: Initializing the Time Synchronizer LKM\n");
   sprintf(gpioName, "gpio%d", gpioTS);           // Create the gpio115 name for /sys/ts_lkm/gpio115

   // create the kobject sysfs entry at /sys/ts_lkm -- probably not an ideal location!
   ts_kobj = kobject_create_and_add("ts_lkm", kernel_kobj->parent); // kernel_kobj points to /sys/kernel
   if(!ts_kobj)
   {
      printk(KERN_ALERT "Time Synchronizer: failed to create kobject mapping\n");
      return -ENOMEM;
   }
   // add the attributes to /sys/ts_lkm/ -- for example, /sys/ts_lkm/gpio388/numberInterrupts
   result = sysfs_create_group(ts_kobj, &attr_group);
   if(result) 
   {
      printk(KERN_ALERT "Time Synchronizer: failed to create sysfs group\n");
      kobject_put(ts_kobj);                          // clean up -- remove the kobject sysfs entry
      return result;
   }
   timeInterrupt.tv_sec = 0.0;
   timeInterrupt.tv_nsec = 0.0;

   gpio_request(gpioTS, "sysfs");       // Set up the gpioTS
   gpio_direction_input(gpioTS);        // Set the time synchronizer GPIO to be an input
   gpio_set_debounce(gpioTS, DEBOUNCE_TIME); // Debounce the time synchronizer with a delay of 200ms
   gpio_export(gpioTS, false);          // Causes gpio115 to appear in /sys/class/gpio
			                    // the bool argument prevents the direction from being changed

   // Perform a quick test to see that the time synchronizer is working as expected on LKM load
   printk(KERN_INFO "Time Synchronizer: The time synchronizer is currently: %d\n", gpio_get_value(gpioTS));

   /// GPIO numbers and IRQ numbers are not the same! This function performs the mapping for us
   irqNumber = gpio_to_irq(gpioTS);
   printk(KERN_INFO "Time Synchronizer: The time synchronizer is mapped to IRQ: %d\n", irqNumber);

   if(!isRising)
   {                           // If the kernel parameter isRising=0 is supplied
      IRQflags = IRQF_TRIGGER_FALLING;      // Set the interrupt to be on the falling edge
   }
   printk(KERN_INFO "isRising: %d \n", isRising );   
   printk(KERN_INFO "IRQF_TRIGGER_RISING: %d \n", IRQF_TRIGGER_RISING );
   printk(KERN_INFO "IRQF_TRIGGER_FALLING: %d \n", IRQF_TRIGGER_FALLING );
   printk(KERN_INFO "IRQflags: %d \n", IRQflags );
   // This next call requests an interrupt line
   result = request_irq(irqNumber,             // The interrupt number requested
                        (irq_handler_t) tsgpio_irq_handler, // The pointer to the handler function below
                        IRQflags,              // Use the custom kernel param to set interrupt type
                        "time_synchronizer_handler",  // Used in /proc/interrupts to identify the owner
                        NULL);                 // The *dev_id for shared interrupt lines, NULL is okay
   return result;
}

/** @brief The LKM cleanup function
 *  Similar to the initialization function, it is static. The __exit macro notifies that if this
 *  code is used for a built-in driver (not a LKM) that this function is not required.
 */
static void __exit ts_exit(void){
   printk(KERN_INFO "Interrupt happens %d times\n", numberInterrupts);
   kobject_put(ts_kobj);                   // clean up -- remove the kobject sysfs entry
   free_irq(irqNumber, NULL);               // Free the IRQ number, no *dev_id required in this case
   gpio_unexport(gpioTS);                  // Unexport the lkm time synchronizer GPIO
   gpio_free(gpioTS);                   // Free the lkm time synchronizer GPIO
   printk(KERN_INFO "Goodbye from the time synchronizer LKM!\n");
}

/** @brief The GPIO IRQ Handler function
 *  This function is a custom interrupt handler that is attached to the GPIO above. The same interrupt
 *  handler cannot be invoked concurrently as the interrupt line is masked out until the function is complete.
 *  This function is static as it should not be invoked directly from outside of this file.
 *  @param irq    the IRQ number that is associated with the GPIO -- useful for logging.
 *  @param dev_id the *dev_id that is provided -- can be used to identify which device caused the interrupt
 *  Not used in this example as NULL is passed.
 *  @param regs   h/w specific register values -- only really ever used for debugging.
 *  return returns IRQ_HANDLED if successful -- should return IRQ_NONE otherwise.
 */
static irq_handler_t tsgpio_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs)
{
   newInterrupt = 1;					// There is new interrupt
   getnstimeofday(&timeInterrupt);         // Get the current time as ts_current
   //printk(KERN_INFO "Interrupt time: %lu.%.9lu \n", timeInterrupt.tv_sec, timeInterrupt.tv_nsec );
   //printk(KERN_INFO "Time Synchronizer: The time synchronizer state is currently: %d\n", gpio_get_value(gpioTS));
   numberInterrupts++;                     // Global counter, will be outputted when the module is unloaded
   //printk(KERN_INFO "Interrupts happens %d times\n", numberInterrupts); // remove in release
   return (irq_handler_t) IRQ_HANDLED;  // Announce that the IRQ has been handled correctly
}

// This next calls are  mandatory -- they identify the initialization function
// and the cleanup function (as above).
module_init(ts_init);
module_exit(ts_exit);
