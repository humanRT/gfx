#!/usr/bin/env python3

import os

def copy_file_with_scp(remote_host, username, remote_file_path, local_directory):
    try:
        # Ensure local directory exists
        if not os.path.exists(local_directory):
            os.makedirs(local_directory)
        
        scp_command = f'scp "{username}@{remote_host}:{remote_file_path}" {local_directory}'
        print(f"-->: {scp_command}")
        os.system(scp_command)
        print("File copied successfully.")
    
    except Exception as e:
        print(f"An error occurred: {e}")


if __name__ == "__main__":
    username = "edgar"             # Replace with the Windows username
    remote_host = "192.168.0.154"  # Replace with the Windows system's IP address
    remote_file_path = "C:/Users/edgar/OneDrive/Desktop/bot_files/gfx/models/CRX10_axis1.glb"
    local_directory = "/home/edgar/Projects/cpp/gfx/models/"  # Replace with your local directory

    copy_file_with_scp(remote_host, username, remote_file_path, local_directory)