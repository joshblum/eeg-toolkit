eeg-toolkit
=================

## Get running in 5 minutes

First, check out eeg-toolkit and launch it locally:

```bash
git clone git@github.com:joshblum/eeg-toolkit.git
cd eeg-toolkit
```

You can either setup the repository to contribute to development or as a
standalone node to use the toolkit. After setting up either kind of service,
you should import data to run the application.

## Production setup

We recommend using the docker images to run a production node. Once the
repository is cloned you can run the following:

```bash
make docker-install
# log out and then log back in again for docker group permission changes to take affect
make docker-run
```

## Development setup

All Python packages are install using `pip` from the `requirements.txt` file.
The commands below install common packages for Linux and run the server.

Note: You need to use `sudo` if you are not working in a
[virtualenv](http://docs.python-guide.org/en/latest/dev/virtualenvs/) when
installing the dependencies.

```bash
# you should first lauch into your virtual env
make install
```

There are three main components that run in the project, the `webapp` server,
the `ws_server` and the `file_watcher`.

The `webapp` server is a Flask server that serves web resources and creates a
websocket to talk to the websocket server, `ws_server`. The `ws_server` is
responsible for computing and serving the EEG data that the frontend then
displays. Below is a screenshot of interface in action:

![screenshot](https://raw.githubusercontent.com/joshblum/eeg-toolkit/master/screenshot.png)

To run the different components you can run the following:

```bash
# run the websocket server
cd toolkit/toolkit
export LD_LIBRARY_PATH=storage/TileDB/core/lib/release
./ws_server
```

```bash
# run the file watcher
cd toolkit/toolkit
python file_watcher.py
```

```bash
# run the webapp server
cd webapp/webapp
python server.py
```

## Importing Data
Data should start in the EDF format. It will automatically be converted for use
when added to the data directory. By default the data directory is
`/home/ubuntu/eeg-data/eeg-data`. This can be changed by modifying the files
`toolkit/toolkit/config.hpp` and `toolkit/toolkit/file_watcher.py`.

The `file_watcher` will monitor the filesystem for files with the extension
`.edf` to convert them for use.

## Experiments
As part of the evaluation of the system we have two experiments to compare
different backend implementations for different workloads. The first experiment
ingests EDF files to convert them to a `StorageBackend` format. This experiment
is run as follows:

```bash
cd toolkit/toolkit/experiments
./ingestion_runner.sh [BinaryBackend|HDF5Backend|TileDBBackend]
```

The second experiment precomputes and stores the spectrogram calculation and
can be run with the following commands:

```bash
cd toolkit/toolkit/experiments
./precompute_runner.sh [BinaryBackend|HDF5Backend|TileDBBackend]
```

Finally, to analyze the results, the following script, also located in
`toolkit/toolkit/experiments` should be used:

```bash
python parse_results.py [BinaryBackend|HDF5Backend|TileDBBackend]
```

This outputs a JSON blob with results data which can then be analyzed. The
experiments expect an EDF file with `mrn="005"` to be present (in the data
directory, described above) to perform the calculations. As they are written,
both experiments need approximately 300GB of disk space to run. Depending on
the backend, the processing can take several days. Grab a coffee after
starting.
