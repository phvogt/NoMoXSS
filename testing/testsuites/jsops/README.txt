To check the opcodes:
- Convert the opcodetests.xls to a tab-separated textfile (e.g., "opcodetests.txt")
- Run all opcode tests. Customize testall.pl and run "./testall.pl <jsop_list.txt"
- Get the xss_connection.log
- Run the script to get the results:

perl jsop_auswertung.pl opcodetests.txt xss_connections.log > results_jsop.txt