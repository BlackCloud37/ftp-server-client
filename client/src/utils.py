from functools import wraps
import traceback
import copy
import socket
def catch_exception(origin_func):
    def wrapper(self, *args, **kwargs):
        try:
            u = origin_func(self, *args, **kwargs)
            return u
        except Exception as e:
            # print(origin_func.__name__ + ": " + e.__str__())
            self.log(str(origin_func.__name__ + ": " + e.__str__()))
            traceback.print_exc()
            return 'an Exception raised.'
    return wrapper

def message_log(origin_func):
    def message_log_wrapper(self, *args, **kwargs):
        # print(origin_func.__name__)
        origin_res = origin_func(self, *args, **kwargs)
        res = copy.deepcopy(origin_res)
        if isinstance(res, tuple) or isinstance(res, list):
            res = [str(each) for each in res]
            res = " ".join(res)
        elif isinstance(res, bytes):
            res = res.decode()

        self.log(origin_func.__name__ + ": " + str(res))
        return origin_res
    return message_log_wrapper

# mode cmd create_conn trans_data
def need_data_conn(origin_func):
    def need_data_conn_wrapper(self, *args, **kwargs):
        assert(self.mode != self.MODE_NONE)
        # set mode
        if self.mode == self.MODE_PASV:
            self.handlePASV()
        elif self.mode == self.MODE_PORT:
            self.handlePORT()

        # send cmd
        step_by = origin_func(self, *args, **kwargs)
        next(step_by)

        # create conn
        if self.mode == self.MODE_PASV:
            self.dataSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.dataSocket.connect((self.pasvDataIp, self.pasvDataPort))
            code, content = self.recvMsg()
            if code != 150:
                return
        elif self.mode == self.MODE_PORT:
            assert(self.portListenSocket)
            
            self.dataSocket, _ = self.portListenSocket.accept()
            code, content = self.recvMsg()
            if code != 150:
                return
            
        self.guiTransStatus(False)
        # recv/send data and do sth.
        res = next(step_by)
        self.guiTransStatus(True)
        # close data session
        if self.dataSocket:
            self.dataSocket.close()
            self.dataSocket = None
        if self.portListenSocket:
            self.portListenSocket.close()
            self.portListenSocket = None
        self.recvMsg()
        return res
    return need_data_conn_wrapper
