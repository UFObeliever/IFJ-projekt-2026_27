import ifj26.*

def small(n: Int): Boolean = n < 4

@main
def main(): Unit =
  var i = 0
  while small(i) do
    print(i)
    i = i + 1
