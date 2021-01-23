#include "s_task.h"
#include <string.h>

#ifdef USE_IN_EMBEDDED

/*******************************************************************/
/* chan for communication between irq and task                     */
/*******************************************************************/

/* Task puts element into chan and waits interrupt to read the chan */
void s_chan_put__to_irq(__async__, s_chan_t *chan, const void *in_object) {
#if 1
    s_chan_put_n__to_irq(__await__, chan, in_object, 1);
#else
    uint16_t end;
    
    S_IRQ_DISABLE();

    while (chan->available_count >= chan->max_count) {
        s_event_wait__from_irq(__await__, &chan->event);
    }

    end = chan->begin + chan->available_count;
    while (end > chan->max_count)
        end -= chan->max_count;

    memcpy((char*)&chan[1] + end * (size_t)chan->element_size, in_object, chan->element_size);
    ++chan->available_count;
    
    S_IRQ_ENABLE();
#endif
}

/* Task waits interrupt to write the chan and then gets element from chan */
void s_chan_get__from_irq(__async__, s_chan_t *chan, void *out_object) {
#if 1
    s_chan_get_n__from_irq(__await__, chan, out_object, 1);
#else
    S_IRQ_DISABLE();

    while (chan->available_count <= 0) {
        s_event_wait__from_irq(__await__, &chan->event);
    }

    memcpy(out_object, (char*)&chan[1] + chan->begin * (size_t)chan->element_size, chan->element_size);

    ++chan->begin;
    while (chan->begin > chan->max_count)
        chan->begin -= chan->max_count;
    --chan->available_count;

    S_IRQ_ENABLE();
#endif
}


/*
 * Interrupt writes element into the chan,
 * return number of element was written into chan
 */
uint16_t s_chan_put__in_irq(s_chan_t *chan, const void *in_object) {
#if 1
    return s_chan_put_n__in_irq(chan, in_object, 1);
#else
    uint16_t end;
    if (chan->available_count >= chan->max_count) {
        return 0;  /* chan buffer overflow */
    }

    end = chan->begin + chan->available_count;
    while (end > chan->max_count)
        end -= chan->max_count;

    memcpy((char*)&chan[1] + end * chan->element_size, in_object, chan->element_size);
    ++chan->available_count;

    s_event_set__in_irq(&chan->event);
    return 1;
#endif
}


/*
 * Interrupt reads element from chan,
 * return number of element was read from chan
 */
uint16_t s_chan_get__in_irq(s_chan_t *chan, void *out_object) {
#if 1
    return s_chan_get_n__in_irq(chan, out_object, 1);
#else
    if (chan->available_count <= 0) {
        return 0;  /* chan buffer is empty */
    }

    memcpy(out_object, (char*)&chan[1] + chan->begin * chan->element_size, chan->element_size);

    ++chan->begin;
    while (chan->begin > chan->max_count)
        chan->begin -= chan->max_count;
    --chan->available_count;

    s_event_set__in_irq(&chan->event);
    return 1;
#endif
}



/* Task puts number of elements into chan and waits interrupt to read the chan */
void s_chan_put_n__to_irq(__async__, s_chan_t *chan, const void *in_object, uint16_t number) {
    while(number > 0) {
        S_IRQ_DISABLE();
        while (chan->available_count >= chan->max_count) {
            s_event_wait__from_irq(__await__, &chan->event);
        }
        s_chan_put_(chan, &in_object, &number);
        S_IRQ_ENABLE();
    }
}

/* Task waits interrupt to write the chan and then gets number of elements from chan */
void s_chan_get_n__from_irq(__async__, s_chan_t *chan, void *out_object, uint16_t number) {
    while(number > 0) {
        S_IRQ_DISABLE();
        while (chan->available_count <= 0) {
            s_event_wait__from_irq(__await__, &chan->event);
        }
        s_chan_get_(chan, &out_object, &number);
        S_IRQ_ENABLE();
    }
}


/*
 * Interrupt writes number of elements into the chan,
 * return number of element was written into chan
 */
uint16_t s_chan_put_n__in_irq(s_chan_t *chan, const void *in_object, uint16_t number) {
    uint16_t count;
    if (chan->available_count >= chan->max_count)
        return 0;

    count = s_chan_put_(chan, &in_object, &number);
    s_event_set__in_irq(&chan->event);

    return count;
}

/*
 * Interrupt reads number of elements from chan,
 * return number of element was read from chan
 */
uint16_t s_chan_get_n__in_irq(s_chan_t *chan, void *out_object, uint16_t number) {
    uint16_t count;
    if (chan->available_count <= 0) 
        return 0;
        
    count = s_chan_get_(chan, &out_object, &number);
    s_event_set__in_irq(&chan->event);
    
	return count;
}

#endif
