# This Python file uses the following encoding: utf-8
import sys
import os
import socket
import re
from utils import catch_exception, need_data_conn, message_log

from PySide2.QtWidgets import QApplication, QWidget, QLabel, QLineEdit, QPushButton, QTextEdit, QTableWidget, QTableWidgetItem, QHeaderView, QAbstractItemView, QInputDialog, QFileDialog, QProgressBar, QMessageBox, QRadioButton
from PySide2.QtCore import QFile, QThread, Signal
from PySide2.QtUiTools import QUiLoader
from PySide2.QtGui import QFont
# TODO: 右键菜单和按钮禁用，文件图标，按钮图标，界面颜色，下载完弹出提示，装模作样STATUS(LOGIN or not)
# TODO: PORT
# TODO: SERVER IP, CODE(such as MKD)
class FTPClient(QWidget):
    MODE_NONE = 0
    MODE_PASV = 1
    MODE_PORT = 2
    # GUI
    def __init__(self):
        super(FTPClient, self).__init__()
        self.setFixedSize(600, 608)
        self.load_ui()
        self.tableWidget = self.findChild(QTableWidget, "tableWidget")
        self.progressBar = self.findChild(QProgressBar, "progressBar")
        self.lineEditUsername = self.findChild(QLineEdit, "lineEditUsername")
        self.lineEditPassword = self.findChild(QLineEdit, "lineEditPassword")
        self.lineEditPort = self.findChild(QLineEdit, "lineEditPort")
        self.lineEditIP1 = self.findChild(QLineEdit, "lineEditIP1")
        self.lineEditIP2 = self.findChild(QLineEdit, "lineEditIP2")
        self.lineEditIP3 = self.findChild(QLineEdit, "lineEditIP3")
        self.lineEditIP0 = self.findChild(QLineEdit, "lineEditIP0")
        self.textEditMessage = self.findChild(QTextEdit, "textEditMessage")
        self.labelStatus = self.findChild(QLabel, "labelStatus")
        self.pushButtonMKD = self.findChild(QPushButton, "pushButtonMKD")
        self.pushButtonRMD = self.findChild(QPushButton, "pushButtonRMD")
        self.pushButtonRename = self.findChild(QPushButton, "pushButtonRename")
        self.pushButtonSTOR = self.findChild(QPushButton, "pushButtonSTOR")
        self.pushButtonLogin = self.findChild(QPushButton, "pushButtonLogin")
        self.pushButtonBack = self.findChild(QPushButton, "pushButtonBack")
        status = False
        self.pushButtonMKD.setEnabled(status)
        self.pushButtonRMD.setEnabled(status)
        self.pushButtonRename.setEnabled(status)
        self.pushButtonSTOR.setEnabled(status)
        self.pushButtonBack.setEnabled(status)

        self.lineEditUsername.setText("anonymous")
        self.labelStatus.setText("Unlogin")
        headers = ["Type", "Permission", "Num", "User", "Group", "Size", "Updated", "Name"]
        self.tableTypeCol = 0
        self.tablePermissionCol = 1
        self.tableNumCol = 2
        self.tableUserCol = 3
        self.tableGroupCol = 4
        self.tableSizeCol = 5
        self.tableUpdatedCol = 6
        self.tableNameCol = 7
        self.tableWidget.setColumnCount(len(headers))
        for idx, header in enumerate(headers):
            headerItem = QTableWidgetItem(header)
            font = headerItem.font()
            font.setBold(True)
            headerItem.setFont(font)
            self.tableWidget.setHorizontalHeaderItem(idx, headerItem)
        self.tableWidget.horizontalHeader().setSectionResizeMode(QHeaderView.ResizeToContents)
        self.tableWidget.verticalHeader().setSectionResizeMode(QHeaderView.ResizeToContents)
        self.tableWidget.setSelectionMode(QAbstractItemView.SingleSelection)
        self.tableWidget.setSelectionBehavior(QAbstractItemView.SelectRows)
        self.tableWidget.setEditTriggers(QAbstractItemView.NoEditTriggers)
        self.tableWidget.setShowGrid(False);
        self.tableWidget.verticalHeader().setHidden(True)
        self.progressBar.setMaximum(100)
        self.progressBar.setValue(0)
        self.findChild(QRadioButton, "radioButtonPASV").setChecked(True)
        self.onPASV()
        self.init_connect()

        # Data section
        self.pwd = "/"
        self.selectedFileItems = None
        self.serverIP = None
        self.serverPort = None
        self.commandSocket = None
        self.dataSocket = None


        self.mode = self.MODE_PASV
        self.pasvDataIp = None
        self.pasvDataPort = None
        self.portListenSocket = None
        self.messages = []

    def load_ui(self):
        loader = QUiLoader()
        path = os.path.join(os.path.dirname(__file__), "form.ui")
        ui_file = QFile(path)
        ui_file.open(QFile.ReadOnly)
        loader.load(ui_file, self)
        ui_file.close()

    def init_connect(self):
        self.pushButtonBack.clicked.connect(lambda: self.handleCWD(".."))
        self.pushButtonLogin.clicked.connect(lambda: self.login())
        self.pushButtonSTOR.clicked.connect(lambda: self.upload())
        self.pushButtonMKD.clicked.connect(lambda: self.makedir())
        self.pushButtonRMD.clicked.connect(lambda: self.remove())
        self.pushButtonRename.clicked.connect(lambda: self.rename())
        self.tableWidget.itemSelectionChanged.connect(self.slotItemChanged)
        self.tableWidget.itemDoubleClicked.connect(self.slotItemDoubleClicked)
        self.findChild(QRadioButton, "radioButtonPASV").toggled.connect(lambda: self.onPASV())
        self.findChild(QRadioButton, "radioButtonPORT").toggled.connect(lambda: self.onPORT())

    


    def log(self, message):
        self.messages = self.messages[-100:]
        self.messages.append(message)
        self.textEditMessage.setPlainText("\r\n".join(self.messages))
        self.textEditMessage.moveCursor(self.textEditMessage.textCursor().End)


    '''
        cmd/data send/recv functions
    '''
    @catch_exception
    def produceCommand(self, command, param=""):
        return command + " " + param + "\r\n"

    @catch_exception
    @message_log
    def recvMsg(self):
        sock = self.commandSocket
        res = b""
        while True:
            buf = sock.recv(1)
            res += buf
            if res[-2:] == b"\r\n":
                break
        if isinstance(res, bytes):
            res = res.decode()
        code, content = int(res[:3]), res[4:].replace("\r\n", "")
        return (code, content)


    @catch_exception
    @message_log
    def sendMsg(self, message):
        sock = self.commandSocket
        if isinstance(message, str):
            bmessage = message.encode()
        sendSize = sock.send(bmessage)
        return message

    @catch_exception
    def recvData(self, toFile=None):
        if not toFile:
            total_data = b''
            while True:
                data = self.dataSocket.recv(10240)
                if not data:
                    break
                else:
                    total_data += data
            return total_data
        else:
            fsize = self.selectedFileItems[self.tableSizeCol]
            if fsize[-1] == 'K':
                fsize = float(fsize[:-1]) * 1024
            elif fsize[-1] == 'M':
                fsize = float(fsize[:-1]) * 1024 * 1024
            elif fsize[-1] == 'G':
                fsize = float(fsize[:-1]) * 1024 * 1024 * 1024
            else:
                fsize = float(fsize)
            self.progressBar.setMaximum(int(fsize))

            with open(toFile, 'wb') as f:
                progress = 0
                while True:
                    self.progressBar.setValue(progress)
                    QApplication.processEvents()
                    data = self.dataSocket.recv(10240)
                    if not data:
                        break
                    f.write(data)
                    progress += len(data)
            return "File write finished."

    @catch_exception
    def sendData(self, message="", fromFile=None):
        if not fromFile:
            if isinstance(message, str):
                bmessage = message.encode()
            sendSize = self.dataSocket.send(bmessage)
            return message
        else:
            with open(fromFile, 'rb') as f:
                self.progressBar.setMaximum(os.stat(fromFile).st_size)
                progress = 0                
                while True:
                    dataChunk = f.read(10240)
                    if dataChunk == b'': #eof
                        break
                    # send the chunk
                    p = 0
                    while p < len(dataChunk):
                        sendSize = self.dataSocket.send(dataChunk[p:])
                        p += sendSize
                    progress += p
                    self.progressBar.setValue(progress)
                    QApplication.processEvents()
            return "File send finished."



    '''
        cmd handlers section
    '''
    def handleTYPE(self, param):
        self.sendMsg(self.produceCommand("TYPE", param))
        code, content = self.recvMsg()

    def handleSYST(self):
        self.sendMsg(self.produceCommand("SYST"))
        code, content = self.recvMsg()
        self.findChild(QLabel, "labelSYST").setText(content)

    def handleUSER(self, param):
        self.sendMsg(self.produceCommand("USER", param))
        self.recvMsg()

    def handlePASS(self, param):
        self.sendMsg(self.produceCommand("PASS", param))
        self.recvMsg()
        self.labelStatus.setText("Logged in")

    def handlePORT(self):
        lfd = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        lfd.connect(("8.8.8.8", 80))
        ip =  lfd.getsockname()[0]
        self.portListenSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.portListenSocket.bind(("0.0.0.0", 0))
        self.portListenSocket.listen(1)
        port = self.portListenSocket.getsockname()[1]
        self.sendMsg(self.produceCommand("PORT", ",".join(ip.split(".")) + "," + str(port//256) + "," + str(port%256)))
        code, content = self.recvMsg()

    def handlePASV(self):
        self.sendMsg(self.produceCommand("PASV"))
        code, content = self.recvMsg()
        splited = re.findall(re.compile(r'[(](.*)[)]', re.S), content)[0].split(',')
        self.pasvDataIp = ".".join(splited[:-2])
        self.pasvDataPort = int(splited[-2]) * 256 + int(splited[-1])
        self.dataSocket = None

    @need_data_conn
    def handleRETR(self, param, toFile):
        yield self.sendMsg(self.produceCommand("RETR", param))
        yield self.recvData(toFile=toFile)

    @need_data_conn
    def handleSTOR(self, param, fromFile):
        yield self.sendMsg(self.produceCommand("STOR", param))
        yield self.sendData(fromFile=fromFile)

    def handlePWD(self):
        self.sendMsg(self.produceCommand("PWD"))
        code, content = self.recvMsg()
        # assert(code == 257)
        self.pwd = content.replace("\"", "")
        if self.pwd[-1] != "/":
            self.pwd += "/"
        # print(self.pwd)
        if self.pwd == "/":
            self.pushButtonBack.setEnabled(False)
        else:
            self.pushButtonBack.setEnabled(True)

    def handleCWD(self, param):
        self.sendMsg(self.produceCommand("CWD", param))
        code, content = self.recvMsg()
        if code == 250:
            self.handlePWD()
            self.handleLIST(self.pwd)
        elif code == 550:
            pass
        else:
            pass

    def handleRMD(self, param):
        self.sendMsg(self.produceCommand("RMD", param))
        code, content = self.recvMsg()
        # print(code, content)
        self.handleLIST(self.pwd)

    def handleMKD(self, param):
        self.sendMsg(self.produceCommand("MKD", param))
        code, content = self.recvMsg()
        # assert(code == 257)
        self.handleLIST(self.pwd)

    def handleRNFR(self, param):
        self.sendMsg(self.produceCommand("RNFR", param))
        self.recvMsg()
    def handleRNTO(self, param):
        self.sendMsg(self.produceCommand("RNTO", param))
        self.recvMsg()

    @need_data_conn
    def handleLIST(self, param):
        yield self.sendMsg(self.produceCommand("LIST", param))
        data = self.recvData().decode()
        for i in range(self.tableWidget.rowCount()):
            self.tableWidget.removeRow(0)
        data.replace("\r", "")
        for row, line in enumerate(data.split("\n")):
            splited = line.split()
            # empty
            if len(splited) == 0:
                continue
            updated = " ".join(splited[5:8])
            filename = " ".join(splited[8:])
            row_data = [splited[0][0]] # first letter is file's type
            for i in range(5):
                row_data.append(splited[i])
            row_data.append(updated)
            row_data.append(filename)
            row_data[1] = row_data[1][1:]
            self.tableWidget.insertRow(row)
            for col, cell_data in enumerate(row_data):
                self.tableWidget.setItem(row, col, QTableWidgetItem(cell_data))
        yield data
    def handleQUIT(self):
        self.sendMsg(self.produceCommand("QUIT"))
        self.recvMsg()
        



    '''
        GUI slots
    '''

    def onPASV(self):
        self.mode = self.MODE_PASV
        self.findChild(QRadioButton, "radioButtonPORT").setChecked(False)

    def onPORT(self):
        self.mode = self.MODE_PORT
        self.findChild(QRadioButton, "radioButtonPASV").setChecked(False)
    
    def login(self):
        status = True
        self.pushButtonMKD.setEnabled(status)
        self.pushButtonRMD.setEnabled(status)
        self.pushButtonRename.setEnabled(status)
        self.pushButtonSTOR.setEnabled(status)
        self.pushButtonBack.setEnabled(status)

        username = self.lineEditUsername.text()
        self.log("Username: %s" % username)
        password = self.lineEditPassword.text()
        self.log("Password: %s" % password)

        self.serverIP = ".".join([self.lineEditIP0.text(), self.lineEditIP1.text(), self.lineEditIP2.text(), self.lineEditIP3.text()])
        self.log("IP: %s" % self.serverIP)
        self.serverPort = int(self.lineEditPort.text())
        self.log("Port: %d" % self.serverPort)
        self.commandSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            self.commandSocket.connect((self.serverIP, self.serverPort))
            self.labelStatus.setText("Connected")
        except Exception as e:
            self.log(e.__str__)
            self.labelStatus.setText("Connect error")
        self.recvMsg()
        self.handleUSER(username)
        self.handlePASS(password)
        self.handlePWD()
        self.handleLIST(self.pwd)
        self.handleSYST()
        self.handleTYPE("I")

    @catch_exception
    def remove(self):
        reply = QMessageBox.question(self, 'Message', f'You sure to remove {self.selectedFileItems[self.tableNameCol]}?',
                                     QMessageBox.Yes | QMessageBox.No, QMessageBox.No)
        if reply == QMessageBox.Yes:
            self.handleRMD(self.pwd + self.selectedFileItems[self.tableNameCol])

    @catch_exception
    def rename(self):
        value, ok = QInputDialog.getText(self, "Rename", "Input the name:", QLineEdit.Normal, "NewName")
        if ok and value:
            fname = self.selectedFileItems[self.tableNameCol]
            self.handleRNFR(self.pwd + fname)
            self.handleRNTO(self.pwd + value)
            self.handleLIST(self.pwd)

    @catch_exception
    def makedir(self):
        value, ok = QInputDialog.getText(self, "Make dir", "Input the dir name:", QLineEdit.Normal, "NewFolder")
        if ok and value:
            self.handleMKD(value)

    def guiTransStatus(self, status):
        self.pushButtonMKD.setEnabled(status)
        self.pushButtonRMD.setEnabled(status)
        self.pushButtonRename.setEnabled(status)
        self.pushButtonSTOR.setEnabled(status)
        self.pushButtonBack.setEnabled(status)
        self.pushButtonLogin.setEnabled(status)

    def closeEvent(self, event):
        self.handleQUIT()
        event.accept()
    def upload(self):
        fromfile, filetype = QFileDialog.getOpenFileName(self,
                                    "Select the file",
                                    "",
                                    "All Files (*)")
        if fromfile:
            fname = fromfile.split("/")[-1]
            self.handleSTOR(self.pwd + fname, fromFile=fromfile)
            self.handleLIST(self.pwd)
            reply = QMessageBox.information(self, 'Greetings', 'Upload finished',
                    QMessageBox.Yes, QMessageBox.Yes)
    def download(self):
        fname = self.selectedFileItems[self.tableNameCol]
        tofile, ok = QFileDialog.getSaveFileName(self,
                                    "Save file to",
                                    f"./{fname}",
                                    "All Files (*)")
        if tofile and ok:
            self.handleRETR(self.pwd + fname, tofile)
            reply = QMessageBox.information(self, 'Greetings', 'Download finished',
                                QMessageBox.Yes, QMessageBox.Yes)

    def slotItemChanged(self):
        selected = self.tableWidget.selectedItems()
        if len(selected):
            self.selectedFileItems = [_.text() for _ in selected]
        # filetype
        ftype = self.selectedFileItems[self.tableTypeCol]
        if ftype == 'd':
            self.pushButtonRMD.setEnabled(True)
        else:
            self.pushButtonRMD.setEnabled(False)


    # means get into dir or download file
    def slotItemDoubleClicked(self, item):
        ftype = self.selectedFileItems[self.tableTypeCol]
        fname = self.selectedFileItems[self.tableNameCol]
        if ftype == 'd': # get into dir
            if self.pwd[-1] == "/":
                self.handleCWD(self.pwd + fname)
            else:
                self.handleCWD(self.pwd + fname)
        else: # download
            self.download()
        
        
if __name__ == "__main__":
    app = QApplication([])
    widget = FTPClient()
    widget.show()
    sys.exit(app.exec_())
