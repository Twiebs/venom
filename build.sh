echo '===================================================================================='
clang++ -std=c++14 -Wall -Wextra -O0 -g venom_platform.cpp -DVENOM_ENGINE -lX11 -ldl -lassimp -o ../build/engine
sh build_game.sh 
