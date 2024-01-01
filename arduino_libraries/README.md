Symlink all the folders here into your Arduino libraries folder. This lets us
"install" libraries that are actually version controlled by this repo.

On Mac, run the following for each folder in `arduino_libraries`:

```bash
ln -s /absolute/path/to/library ~/Documents/Arduino/libraries/
```

On Windows, run the following for each folder in `arduino_libraries` (in an Administrator command prompt):

```
mklink /D %USERPROFILE%\OneDrive\Documents\Arduino\libraries\library_name C:\absolute\path\to\library
```
