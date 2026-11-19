import ifj26.*

def sum10: Int =
  var s = 0
  var i = 1
  while i <= 10 do
    s = s + i
    i = i + 1
  s

@main
def main(): Unit =
  print(sum10)
