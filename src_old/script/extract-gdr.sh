cat $1 | grep ^/ | sed 's/,x,,,,,,[0-9]*,[0-9]*,,,//' | sed 's/^[a-zA-Z_\/0-9\-]*,[0-9]*$/&,-1,-1/' 
