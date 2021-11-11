import zmq
import numpy as np
import struct
import pandas as pd
from datetime import date, datetime
import time

zmq__context = zmq.Context()
zmq__socket = zmq__context.socket(zmq.SUB)
zmq__socket.connect("tcp://localhost:5001") # connect, not bind, the PUB will bind, only 1 can bind
zmq__socket.setsockopt(zmq.SUBSCRIBE, b'') # subscribe to topic of all (needed or else it won't work)
zmq__poller = zmq.Poller()
zmq__poller.register(zmq__socket, zmq.POLLIN)
int__poll_iteration = 10000
dict__data = dict()
# TODO: implement samle rate timedelta calcualtion
int_sample_rate = 2000000

np__sorted_main_data = 0

start_time = time.time()

seconds = 10
print("Retrive procedere")
while True:
#for i in range(int__poll_iteration):
    current_time = time.time()
    elapsed_time = current_time - start_time
    if elapsed_time > seconds:
        break
    zmq__sockets = dict(zmq__poller.poll())
    if zmq__socket in zmq__sockets and zmq__sockets[zmq__socket] == zmq.POLLIN:
        list__data = zmq__socket.recv_multipart(copy=False) # Handles multi-part-msg
        np__post_msg = None
        # TODO: timestamp here
        for data in list__data:
            np__past_msg = None
            if isinstance(data, zmq.sugar.frame.Frame):
                data: zmq.sugar.frame.Frame = data
                #int__size_of_complex128_in_bytes = 16
                int__size_of_float64_in_bytes = 8
                int__size_of_buffer_in_bytes = len(data.buffer)
                int__number_of_datasets = int(int__size_of_buffer_in_bytes / int__size_of_float64_in_bytes) #int__size_of_complex128_in_bytes)
                try:
                    np__past_msg = np.frombuffer(data.bytes, np.float, int__number_of_datasets) #dtype=np.complex128, count=int__count)
                except Exception as e:
                    print(e)
            else:
                buf = memoryview(data)
                try:
                    np__past_msg = np.frombuffer(buf, dtype=np.float, count=struct.calcsize(buf)) #count=sys.getsizeof(np.complex128))
                except Exception as e:
                    print(e)

            if isinstance(np__post_msg, np.ndarray):
                np__post_msg = np.append(np__post_msg, np__past_msg)
            else:
                np__post_msg = np__past_msg

        if isinstance(np__post_msg, np.ndarray):
            np__sorted_main_data = np.append(np__sorted_main_data, np__post_msg)
        else:
            np__sorted_main_data = np__post_msg


dict__data = {"float_values": np__sorted_main_data}
print(dict__data)
df = pd.DataFrame.from_dict(dict__data, dtype=np.float).T
datetime__timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
df.to_hdf(f"example_data_energy_saving_lamp_tube_TinS{seconds}_{datetime__timestamp}.h5",
key="df",
index=False)
