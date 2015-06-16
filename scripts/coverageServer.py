#!/usr/bin/python
from flask import *
from functools import wraps
from subprocess import call
from werkzeug import secure_filename
import json
import time
import os
import shutil
import tarfile

##################### DESCRIPTION #################
#This is the server to which coverage data is uploaded from travisCI.
#need to replace USER and PASSWORD with an actual username and passsword used to autheticate in /api
#Stored here, because it needs to be in version control somewhere
#
# Example of running the server
#uwsgi -s 127.0.0.1:3013 -w coverageServer:app --chmod=666 --post-buffering=81 --daemonize ./log
#where nginx is expecting uwsgi traffic at 127.0.0.1:3013


#sample upload command
#curl --form "file=@coverage.tar.gz" -u USER:PASSWORD localhost:5000/api




UPLOAD_FOLDER = '.'

#enable upload only every 5 minutes

app = Flask(__name__)
app.config['UPLOAD_FOLDER'] = UPLOAD_FOLDER
app.config['MAX_CONTENT_LENGTH'] = 16 * 1024 * 1024

TIMEOUT = 0


def check_auth(username, password):
    return username == 'USER' and password == 'PASSWORD'


def authenticate():
    """Sends a 401 response that enables basic auth"""
    return Response(
    'Could not verify your access level for that URL.\n'
    'You have to login with proper credentials', 401,
    {'WWW-Authenticate': 'Basic realm="Login Required"'})


def requires_auth(f):
    @wraps(f)
    def decorated(*args, **kwargs):
        auth = request.authorization
        if not auth or not check_auth(auth.username, auth.password):
            return authenticate()
        return f(*args, **kwargs)
    return decorated

@app.route("/<path:path>")
def serve_page(path):
	return send_from_directory('./coverage', path)

#check that the tar file has only one efolder named coverage
def check_tar(tar):
	coverage_dir_num = 0
	for tarinfo in tar.getmembers():
		if 'coverage' in tarinfo.name and tarinfo.isdir(): 
			coverage_dir_num += 1
		elif not 'coverage/' in tarinfo.name:
			return False
	return coverage_dir_num == 1

@app.route("/api", methods=['POST'])
@requires_auth
def upload_coverage():
    global TIMEOUT
#only allow uploads every 5 minutes (a bit less than usuall travisCI build)
    if time.time() - TIMEOUT < 300:
	    return "Time Out"
    if request.method == 'POST':
        file = request.files['file']
	if file.filename == "coverage.tar.gz":  
          filename = secure_filename(file.filename)
          file.save(os.path.join(app.config['UPLOAD_FOLDER'], filename))
          tar = tarfile.open("./coverage.tar.gz")
	  if not check_tar(tar):
		return "NOK"
#remove the previous coverage folder if it existis
      if os.path.isdir("./coverage"):
    	    shutil.rmtree("./coverage")
      tar.extractall()
      tar.close()
	  TIMEOUT = time.time()
      return "OK"
    return "NOK"

if __name__ == "__main__":
    app.debug = True
    app.run(host="0.0.0.0")
