@brief BLE_protocols: provides an easy to use framework for BLE communication on Arduino without using an RTOS.
@details
Overview
========
There are two pieces to this project:
1. A framework for specifying protocols that is executable
2. A set of functions to implement BLE protocols using ST's BlueRNG STBLE library for Arduino

A Framework for Specifying Protocols
==================================== 
Although FreeRTOS will run on Arduino systems that are capable enough to support BLE, the "native" way to run 
Arduino sketches is to have a single loop that runs all worker "threads" by calling the same functions repeatedly;
each time one of these worker thread functions is executed, it performs one small step of something that is much
larger and can't execute all at once. Communications protocols are a prime example of larger processes that can't
run all at once because they have to wait for the other side to do something and send something back, which they
then respond to, and wait for the other side again. In order for one of these functions to "wait" for the other side, 
they return from their function and when called again they pick up where they left off and do a little bit more.

Bluetooth Low Energy (BLE) is a good example of being very protocol intensive, meaning that these systems usually
don't do a lot of heavy processing but what they do do is do that processing in very small pieces. Servers send out
an advertisement, wait, send it again, wait, ..., respond to a connection request, wait, respond to an attribute read
or write, wait, etc. 

A somewhat simplistic way to implement BLE in Arduino without multitasking is to create large case statements that 
respond appropriately to whatever message is received. The problem with this approach is that it can be very messy
to keep track of the overall state machine (i.e., protocol) that is in play at any given time. In other words, each
case statement has to look at the overall state of the system, whether a discovery is being done, a finding or
characteristics, whether a connection has already been made, etc. In this type of design, one is forced to look at
the protocols being implemented "from the bottom up", meaning that what is explicit is the response in each case
statement. 

An example of a tops down defined protocol is calling a friend on the telephone to ask if they are coming
to your party tonight:
1. Start phone application on smart phone)
2. Expect keypad and enter phone number
3. Hit send to dial number
4. Expect your friend to say hello and respond with "this is ___; how are you"
5. Expect an answer like "fine", "ok", "doing good" and respond with "glad to hear that".
6. Say "just wondering if you will be able to make the party tonight" and expect an answer like "yes" or "no, have a conflict tonight"
7. Respond with "glad to hear that" or "sorry to hear that" 
8. Say "have a good" day and expect a response like "you too, bye"
9. Expect them to hang up or say "bye" and hang up

Now a "bottoms up" definition of the same protocol would look something like:

    Whenever I hear "hello":
    If I've entered the phone number and am waiting on a response then say "this is ___; how are you, but if I am in the middle of talking
    to them, then say "can you not hear me?". Otherwise this is an error.

    Whenever I hear "yes":
    If I've asked them if they are coming to the party tonight then respond with "glad to hear that", but if I am still in the middle of talking
    to them then they are just letting me know they are still there and they heard me, so continue. Otherwise this is an error.

    ...

Clearly a key thing missing in the bottoms up definition is an explicit statement on the order of things. Instead, the order is implicit,
meaning you need to understand all the rules and how they all fit together to glean the order of steps. Also, the bottoms up
definition focuses a lot on unexpected things, whereas the tops down definition focuses on what is expected. In other words, the tops down
approach emphasizes the mainstream protocol whereas the bottoms up definition gives equal weight to mainstream and all the various unexpected things,
i.e., both expected and unexpected responses are intermixed.

The goal of the protocol framework is to provide a domain specific language for specifying protocols that is executable yet closely matches
the "natural" definition of a protocol in terms of actions and expected responses, allowing one to focus on the mainstream protocol but still
handling all the unexpected things too, and it needs to be very lightweight to run in a tiny embedded system without an RTOS.

The implementation of this framework is in protocol.h/.cpp and production.h/.cpp. They allow the example above to be written in the following style:

    protocol(Call_Friend_Protocol)
      perform:(start_phone_application)
	    expect(keypad, And_Do(enter_phone_number))
  	  run_production
	  perform(hit_send)
	    expect(hear_hello, And_Do(respond_with), With("this is ___; how are you")
	  run_production
	  ...
	end_protocol
	
	...
	
	void configure_unexpected_events() {
	  expect_globally(busy_signal, And_Do(hang_up))
	  ...
	}
	
	void setup() {
	  ...
	  configure_unexpected_events();
	  Call_Friend_Protocol();
	  ...
	}
	
This creates a protocol function called Call_Friend_Protocol. 
"Perform" and "Expect" define what needs to be done and "Run_Production" leaves the protocol function to have the 
framework handle that step of the protocol and then "resume" this protcol once that production step has been completed.
(The keyword "production" was chosen because this is similar to rule based production systems like OPS5 and CLIPS.) 
Expect globally defines expectations that are checked if no specific expect matches the event. (This is what allows the protocol to focus on more expected events.)
Then in the setup function, the Call_Friend function can be called to start the protocol.  

See protocol.h/.cpp and production.h/.cpp for more details and additonal kewords (like repeat_until). See ble_protocols_source.txt for a complete working Arduino example.


Implementing BLE protocols using ST's BlueRNG STBLE library for Arduino
======================================================================= 

ST's STBLE library for Arduino provides many functions, events, and data structures for implmenting BLE clients and servers. Initially I used it in an application
that ran on Tiny Circuits TinyScreen+ with their Bluetooth Low Energy TinyShield to scan for devices of interest, and then enumerate their services and characteristics. 
I attempted to create this using a hierarchical set of case statements and state variables to go through the various steps of discovering devices, connecting to
each one, enumerating their services, enumerating each service's characteristics, etc. This did not go well. It was a mess with lots of bugs that were hard to
figure out because the logic was so spread out between different case statements; fixing one bug would create another bug in a different context. Invariably,
unexpected events would arise and it was hard to handle them correctly all in all contexts. Eventually I realised I really needed to start over and use a completely
different approach. This led to the creation of the protocol/production framework. Implementing my application using this framework went very well - the logic was
straightforward and the bugs were few and easy to find & fix.

To enable protocols for BlueRNG to be written easily, this project has the following:

1. procedures.h/.cpp which provides wrapper functions to initiate (perform) common actions like discovery and finding services that are usable directly by protocol functions
2. get_data.h/.cpp which provides wrapper functions to get data from BlueRNG events that are usable directly by productions
3. HCI.h/.cpp which provides global event handling for all the "unexpected" events that can occur + some wrappers for general HCI functions with error handling adde5
4. addrs.h/.cpp which provides a simple database of devices found and their addresses
5. db.h/.cpp which provides a simple database of services and characteristics found for devices
6. dbprint.h/.cpp which provides a debug trace library for selective printing of debug information


Current Status
==============
The level of functionality is sufficient to discovery BLE devices and enumerate their services and characteristics. This is very generic functionality that would be useful
and a good starting point for any client applications (central role). A todo is to add some generic functionality more suitable for server applications (peripherals).

Documentation
=============
A Doxygen configuration file called Doxyfile can be used to create documentation for the project. Note that Doxygen does not understand the protocol language so it produces 
little useful information for the example protocol file in ble_protocols.ino. However, the souce code should be easy to follow, so a special documentation file is provided
that shows this as formatted source code. So if you view the doxygen generated documentation, refer to ble_protocols_source.txt instead of ble_protocols.ino 
to follow the example in the protocol language.


License
=======
All source code in this project is freely available under MIT license in the hopes that others may find it useful and avoid the nightmare of bottoms-up
protocol implementations.



fine-print: Copyright (c) David Hamilton <david@davidohamilton.com>. 
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
This software may be freely copied and used under MIT license (see LICENSE.txt in root directory).

