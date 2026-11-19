import ifj26.*

@main
def main(): Unit =
  var i = 0
  var j = 10
  while i < 3 || j > 8 do
    i = i + 1
    j = j - 1
  print(i)
  print(",")
  print(j)
