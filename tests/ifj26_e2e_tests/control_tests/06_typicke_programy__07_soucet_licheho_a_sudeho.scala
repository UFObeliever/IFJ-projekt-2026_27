import ifj26.*

@main
def main(): Unit =
  var odd = 0
  var even = 0
  for i <- 1 to 10 do
    if i / 2 * 2 == i then even = even + i else odd = odd + i
  print(odd)
  print(" ")
  print(even)
