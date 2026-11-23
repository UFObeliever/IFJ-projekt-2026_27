import ifj26.*

def even(n: Int): Boolean = n / 2 * 2 == n

@main def main(): Unit =
  var i = 0
  while i < 6 do
    if even(i) then
      print(i)
    i = i + 1
  print("\n")
