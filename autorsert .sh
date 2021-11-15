echo "-------Begin-------"
git log -5 --pretty=oneline

read -p "head:" head
git reset --hard $head
echo "--------End--------"
read -p "Enter a number:" num