::#!/bin/bash 注释 暂时有问题CMD不能到git里面来
echo "-------Begin-------"
git log -5 --pretty=oneline
git status
git add .
read -p "提交信息:" commitinfo
git commit -m $commitinfo
git push -u origin master
git log -5 --pretty=oneline
echo "--------End--------"
read -p "Enter a number:" num