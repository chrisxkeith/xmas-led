# Try to convert paths from Linux to Windows
target=${HOME}/Documents/Github/xmas-led/.vscode/c_cpp_properties.json.win
cat ${HOME}/Documents/Github/xmas-led/.vscode/c_cpp_properties.json | \
sed -e s'/\/home\/ck\/.arduino15\//C:\\\\Users\\\\chris\\\\AppData\\\\Local\\\\Arduino15\\\\/' | \
sed -e s'/\/home\/ck\/Arduino\/libraries\//C:\\\\Users\\\\chris\\\\Arduino\\\\libraries\\\\/' | \
sed -e s'/\//\\\\/'g > $target

# To see if it worked
grep home $target
