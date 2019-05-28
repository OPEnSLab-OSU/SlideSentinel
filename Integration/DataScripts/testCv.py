#!/usr/bin/python
import xlwt


wb = xlwt.Workbook()
ws = wb.add_sheet('A Test Sheet')

ws.write(0, 0, "Longitude")

ws.write(1, 0, 1)
ws.write(2, 0, 1)
ws.write(3, 0, xlwt.Formula("A3+B3"))

wb.save('example.xls')
