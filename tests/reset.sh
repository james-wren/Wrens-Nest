sudo rm /usr/local/bin/wn
rm -rf ~/.wrens_nest
ssh -i ~/.ssh/scraper.key ubuntu@129.80.190.124 "rm -rf ~/.wrens_nest"
