#include <pololu/orangutan.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>  // this lets us refer to data in program space (i.e. flash)

#define DEBUG

const char music[] PROGMEM =
  /* 1 (drum pickup)
     2 (intro, 1st half)
     3 (intro, 2nd half)
     4 we're no strangers to love
     5 you know the rules and so do i (do i do i)
     6 a full commitment's what i'm thinking of
     7 you wouldn't get this from any other guy
     8 i just wanna tell you how i'm feeling
     9 gotta make you understand
    10 never gonna give you up
    11 never gonna let you down
    12 never gonna run around and desert you
    13 never gonna make you cry
    14 never gonna say goodbye
    15 never gonna tell a lie and hurt you
  */
  "! O4 T114 L16 V13 MS c+drc+cc<b<a+<a<g+ \
  O5 L4 V11 ML c+.d+.<g+ d+.f. L16 g+f+fc+ L4 c+.d+.<g+<g+.r L16 V13 <g+<g+<a+c+<a+c+ \
  L4 V11 c+.d+.<g+ d+.f. L16 g+f+fc+ L4 c+.d+.<g+<g+.r8 L16 V13 c+c+r8c+c+rc+ \
  O4 L8 V15 r4<a+cc+c+d+c.<a+16<g+<g+2r4 \
  r<a+<a+cc+<a+r<g+g+rg+d+ V11 g+d+ V10 g+d+ \
  V15 r<a+<a+cc+<a+c+d+rc<a+<a+16<g+4.r. \
  r<a+<a+cc+<a+<g+4d+d+d+fd+4.r \
  c+2c+d+fc+d+d+d+fd+<g+16<g+4.r16 \
  r4.<a+cc+<a+rd+fd+4. \
  L16 <g+<a+c+<a+ MS f8.f8. ML d+4. \
  <g+<a+c+<a+ MS d+8.d+8. ML c+8.c16<a+8 \
  <g+<a+c+<a+c+4d+8c8.<a+<g+4<g+8d+4c+4.r8 \
  <g+<a+c+<a+ MS f8.f8. ML d+4. \
  <g+<a+c+<a+g+4c8c+8.c<a+8 \
  <g+<a+c+<a+c+4d+8c8.<a+<g+4<g+8d+4c+4 V13 c+c+r8c+c+rc+";
//                                         (intro, 1st half)                                                                        (intro, 2nd half)                                                     we're no strangers to love              you know the rules and so do i          a full commitment's what i'm think...   you wouldn't get this from any other... i         just wanna tell you how i'm feeling     gotta make you understand     give you up         let you down        run around and desert you               make you cry       say goodbye         tell a lie and hurt you
//                                    0    1    2    3    4    5    6    7    8    9   10   11   12   13   14   15   16   17   18   19   20   21   22   23   24   25   26   27   28   29   30   31   32   33   34   35   36   37   38   39   40   41   42   43   44   43   46   47   48   49   43   51   52   53   54   55   56   57   58   59   60   61   62   63   64   65   66   67   68   69   70   71   72   73   74   75   76   77   78   79   80   81   82   83   84   85   86   87   88   89   90   91   92   93   94   95   96   97   98   99  100  101  102  103  104  105  106  107  108  109  110  111  112
const int  left_motor_speed[113] = { 46, -43, -43,  43,  43, -43, -43,  43,  43, -43, -43,  43,  43, -43, -43,  60, -60, -43, -43,  43,  43, -43, -43,  43,  43, -43, -43,  43,  43, -43, -43, 120, 120, -43, -43,  43,  43, -43, -43,  43,  43, -43, -43,  43,  43, -43, -43,  60, -60,  43,  43, -43, -43,  43,  43, -43, -43,  43,  43, -43, -43,  43,  43, -43, -43,  43,  43, -43, -43,  43,  43, -43, -43,  80, -80,  43,  43, -43, -43,  43,  43, -80,  80, -43, -43,  80, -80,  43,  43,  60, -60,  60, -60, -43, -43,  43,  43, -80,  80, -43, -43,  80, -80,  43,  43,  60, -60,  60, -60, -43, -43, 120, 120};
const int right_motor_speed[113] = {-46,  43,  43, -43, -43,  43,  43, -43, -43,  43,  43, -43, -43,  43,  43,  60, -60,  43,  43, -43, -43,  43,  43, -43, -43,  43,  43, -43, -43,  43,  43,-120,-120,  43,  43, -43, -43,  43,  43, -43, -43,  43,  43, -43, -43,  43,  43,  60, -60, -43, -43,  43,  43, -43, -43,  43,  43, -43, -43,  43,  43, -43, -43,  43,  43, -43, -43,  43,  43, -43, -43,  43,  43, -80,  80, -43, -43,  43,  43, -43, -43,  80, -80,  43,  43, -80,  80, -43, -43,  60, -60,  60, -60,  43,  43, -43, -43,  80, -80,  43,  43, -80,  80, -43, -43,  60, -60,  60, -60,  43,  43,-120,-120};
const unsigned int BEAT_TIMEOUT_START = 5140; // 20000000 / 8 / 256 / 5140 = ~1.9 Hz = 114 bpm

static unsigned int beat_timeout;
static unsigned char beat_count;
static unsigned char led_select;

int main() {

  clear();

	TCCR0B = 0x2; // timer 0 clock select FCPU / 8


  while(1) {
    beat_timeout = BEAT_TIMEOUT_START * 2.5; // pickup is 10 sixteenth notes
    beat_count = 0;
    led_select = 0;

    wait_for_button(BUTTON_B);

    delay_ms(1000);

    play_from_program_space(music);

  	TIFR0 |= 0xFF;          // clear any pending t1 overflow int.
    TIMSK0 = (1 << TOIE0);  // enable interrupt

    set_motors(left_motor_speed[0], right_motor_speed[0]);

    while(is_playing());
  }
}

ISR (TIMER0_OVF_vect)
{
  if (beat_timeout == 0) {

    // end of song
    if (++beat_count > 112) { // 112 + pickup
      TIMSK0 = 0; // disable interrupt

      set_motors(0, 0);

    } else {

#ifdef DEBUG 
      clear();
      lcd_goto_xy(0, 0);
      print_long(beat_count);
#endif

      // turn on led
      if(led_select) {
        red_led(1);
      } else {
        green_led(1);
      }
      set_motors(left_motor_speed[beat_count], right_motor_speed[beat_count]);

      beat_timeout = BEAT_TIMEOUT_START;
      if ((beat_timeout & 0x3) == 0)
        beat_timeout++;  // effective beat_timeout = 5140.25
    }

  } else if (beat_timeout == 4500) {
    //turn off led
    if(led_select) {
      red_led(0);
    } else {
      green_led(0);
    }
    led_select = !led_select;

  }

  beat_timeout--;
}
