import 'package:flutter/material.dart';

class HomeScreen extends StatefulWidget {
  const HomeScreen({Key? key}) : super(key: key);

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('IR Remote'),
      ),
      body: Container(
          width: 200,
          padding: EdgeInsets.all(40),
          child: Column(
            children: const [
              Expanded(child: TextField()),
              Expanded(child: TextField()),
              Spacer()
            ],
          )),
      floatingActionButton: FloatingActionButton(
        onPressed: () {
          print('hello');
        },
        tooltip: 'Send IR',
        child: const Icon(Icons.send),
      ),
    );
  }
}
