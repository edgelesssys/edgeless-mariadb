# edgeless-mariadb

## Build

* Set `-DWITH_EDB=ON` if you want to build mariadb for EDB
* Set `-DWITH_EROCKS=ON` if you want to build mariadb with edgeless-rocksdb

```sh
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug [-DWITH_EDB=ON] [-DWITH_EROCKS=ON] ..
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
