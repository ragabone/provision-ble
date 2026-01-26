# Create  
/etc/systemd/system/provision-ble.service
# Enable and Start
sudo systemctl daemon-reexec
sudo systemctl daemon-reload
sudo systemctl enable provision-ble
sudo systemctl start provision-ble
# check status
sudo systemctl status provision-ble

# view logs 
journalctl -u provision-ble -f
