import ifj26.*

@main
def main(): Unit =
  var i = 0
  var s = 0
  while i < 10 && s < 20 do
    s = s + i
    i = i + 1
  print(i)
  print(",")
  print(s)
