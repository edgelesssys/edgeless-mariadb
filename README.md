# edgeless-mariadb
This is a modified [MariaDB](https://github.com/MariaDB/server) designed to run in an SGX enclave. It is used by [EDB](https://github.com/edgelesssys/edb).

## Build
```sh
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j`nproc`
```

## Test
Some tests require a running mariadbd:
```sh
cd build
scripts/mysql_install_db --srcdir=.. --auth-root-authentication-method=normal --no-defaults
sql/mariadbd --datadir=./data
```

Then just run ctest:
```sh
cd build
ctest --output-on-failure
```
