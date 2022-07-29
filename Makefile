all:
	@echo "Creating dataServer and remoteClient executables..."
	@cd ./server/ && $(MAKE) --no-print-directory && cd ..
	@cd ./client/ && $(MAKE) --no-print-directory && cd ..

clean:
	@cd ./server/ && $(MAKE) clean --no-print-directory && cd ..
	@cd ./client/ && $(MAKE) clean --no-print-directory && cd ..
