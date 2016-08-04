#!/usr/bin/env python3
from subprocess import Popen, PIPE
from multiprocessing import Process, Queue
import re

re_motion_file = re.compile(r'^motion: file \"(?P<file>.*)\", folder \"(?P<folder>.*)\", pixels \"(?P<pixels>.*)\"$')
re_gdrive_mkdir = re.compile('^Directory (?P<folder_id>.*) created$')



def motion_detection(q):
    with Popen(['../motion_detection',
                '-d', '0', '-w', '1920', '-H', '1080', '-f', '5', '-t', '0'],
               stdout=PIPE, bufsize=1, universal_newlines=True) as motion:
        for line in motion.stdout:
            print('new line')
            if re_motion_file.match(line):
                imgfile = re_motion_file.search(line).group('file')
                folder = re_motion_file.search(line).group('folder')
                pixels = re_motion_file.search(line).group('pixels')
                print('-> file_to_drive(imgfile, folder)')
                q.put([imgfile, folder, pixels])
                print('<- file_to_drive(imgfile, folder)')
            else:
                print(line)


class GDrive:
    def __init__(self, gdrive_cmd, current_folder, current_folder_id):
        self.gdrive_cmd = gdrive_cmd
        self.current_folder = current_folder
        self.current_folder_id = current_folder_id
    
                
    def folder(self, folder):
        print('--> folder_to_gdrive(folder)')
        with Popen([self.gdrive_cmd, 'mkdir', '-p', '0B7-MlmFLTTnjcE9POHFsUkFsNlk', folder],
                   stdout=PIPE, universal_newlines=True) as gdrive_mkdir:
            print('---> folder_to_gdrive(folder)')
            output = gdrive_mkdir.stdout.read()
            print(output)
            if re_gdrive_mkdir.match(output):
                print(output)
                self.current_folder_id = re_gdrive_mkdir.search(output).group('folder_id')
                self.current_folder = folder
                print(self.current_folder_id)
            else:
                print('motion: failed to create google drive folder: ' + output)

    def file(self, imgfile, folder):
        print('-> file_to_drive(' + imgfile + ', ' + folder + ')')
        if folder != self.current_folder:
            print('-> folder_to_gdrive(folder)')
            self.folder(folder)
            print('<- folder_to_gdrive(folder): ' + self.current_folder_id)
            print('debug: ' + self.gdrive_cmd + ' upload -p' + self.current_folder_id +
                  ' ' + imgfile)
        with Popen([self.gdrive_cmd, 'upload', '-p', self.current_folder_id, imgfile],
               stdout=PIPE, universal_newlines=True) as gdrive_upload:
            output = gdrive_upload.stdout.read()
            print(output)
        

if __name__ == '__main__':
    gdrive = GDrive('gdrive', 'None', '')
    q = Queue()
    p = Process(target=motion_detection, args=(q,))
    p.start()
    while True:
        motion_data = q.get()
        print(motion_data)
        imgfile = motion_data[0]
        folder = motion_data[1]
        pixels = motion_data[2]
        gdrive.file(imgfile, folder)
    p.join()
